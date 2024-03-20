#pragma once

#include <cstdint>
#include <cstring>

#include <atomic>

#include <clean-core/alloc_vector.hh>
#include <clean-core/allocator.hh>
#include <clean-core/bit_cast.hh>
#include <clean-core/intrinsics.hh>
#include <clean-core/new.hh>

namespace cc
{
namespace detail
{
void radix_sort(uint32_t* __restrict a, uint32_t* __restrict temp, size_t n);
}

/// Fixed-size object pool, synchronized and lock-free
/// O(1) acquire, release and size overhead
/// Pointers remain stable
/// acquire() and release() fully thread-safe
/// (access to underlying memory unsynchronized)
template <class T, bool GenCheckEnabled>
struct atomic_linked_pool
{
    using handle_t = uint32_t;

    void initialize(size_t size, cc::allocator* allocator = cc::system_allocator)
    {
        if (size == 0)
            return;

        if constexpr (sc_enable_gen_check)
        {
            CC_ASSERT(size <= sc_max_size_with_gen_check && "atomic_linked_pool size too large for index type");
        }
        else
        {
            CC_ASSERT(size <= sc_max_size_without_gen_check && "atomic_linked_pool size too large for index type");
        }

        CC_CONTRACT(size > 1 && "pool too small");
        CC_CONTRACT(allocator != nullptr && "no allocator provided");
        CC_ASSERT(_pool == nullptr && "re-initialized atomic_linked_pool");

        _alloc = allocator;

        _pool_size = size;

        // allocate pool nodes
        _pool = reinterpret_cast<T*>(_alloc->alloc( //
            sizeof(T) * _pool_size,                 //
            cc::max<size_t>(64, alignof(T))));

        // allocate free list
        _free_list = reinterpret_cast<int32_t*>(_alloc->alloc( //
            sizeof(int32_t) * (_pool_size - 1),                //
            64));

        // initialize free list
        for (int32_t i = 0; i < _pool_size - 1; ++i)
        {
            _free_list[i] = i + 1;
        }
        // initialize free list tail
        _free_list[_pool_size - 1] = -1;

        // initialize generation handles
        if constexpr (sc_enable_gen_check)
        {
            _generation = reinterpret_cast<internal_handle_t*>(_alloc->alloc(sizeof(internal_handle_t) * _pool_size, alignof(internal_handle_t)));
            std::memset(_generation, 0, sizeof(internal_handle_t) * _pool_size);
        }

        // initialize first free node index
        VersionedIndex head;
        head.set_index(0);
        _first_free_node.store(head);

        // initiale destructor function pointer
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            // set up the function pointer now, as T is complete in this function (unlike in the dtor and this->_destroy())
            _fptr_call_all_dtors = +[](atomic_linked_pool& pool) { pool.iterate_allocated_nodes([](T& node) { node.~T(); }); };
        }
        else
        {
            _fptr_call_all_dtors = nullptr;
        }
    }

    void destroy() { _destroy(); }

    /// acquire a new slot in the pool
    [[nodiscard]] handle_t acquire()
    {
        bool cas_success = false;
        int32_t acquired_node_index = -1;

        // acquire-candidate: the current value of _first_free_node
        VersionedIndex acquired_node_gen_index = _first_free_node.load(std::memory_order_acquire);
        VersionedIndex next_node_gen_index;
        do
        {
            // we loaded the first free node to receive a _candidate_ for the node we will actually aquire
            acquired_node_index = acquired_node_gen_index.get_index();
            CC_ASSERT(acquired_node_index != -1 && "atomic_linked_pool is full");

            // load the next-index of the candidate node
            int32_t* const p_free_list = &_free_list[acquired_node_index];
            // force an atomic load by adding 0
            int32_t const free_list_value = cc::intrin_atomic_add(p_free_list, 0);

            // the new _first_free_node will point to that next-index, with a bumped version
            // the version bump is crucial to avoid the ABA problem
            // if we were to _only_ CAS on the index or a pointer, a different thread could acquire AND free a different node in the meantime
            // meaning the CAS would succeed even though the intermediate work was raced
            next_node_gen_index = acquired_node_gen_index;
            next_node_gen_index.set_index(free_list_value);

            // run the CAS on the two versioned indices
            cas_success = _first_free_node.compare_exchange_weak(acquired_node_gen_index, next_node_gen_index, std::memory_order_acquire, std::memory_order_acquire);

            // if we fail, acquired_node_gen_index got rewritten by the CAS - retry
            // in this case, a different thread was faster
        } while (!cas_success);

        T* const acquired_node = &_pool[acquired_node_index];

        // call the constructor
        if constexpr (!std::is_trivially_constructible_v<T>)
            new (cc::placement_new, acquired_node) T();

        // construct a handle
        return _construct_handle(acquired_node_index);
    }

    /// release a slot in the pool, destroying it
    void release(handle_t handle)
    {
        uint32_t const real_index = _read_handle_index_on_release(handle);
        _release_node(real_index);
    }

    /// access a slot
    CC_FORCE_INLINE T& get(handle_t handle) { return _pool[_read_handle_index(handle)]; }

    /// access a slot
    CC_FORCE_INLINE T const& get(handle_t handle) const { return _pool[_read_handle_index(handle)]; }

    /// test if a handle references a currently live slot,
    /// requires GenCheckEnabled (template parameter)
    bool is_alive(handle_t handle) const
    {
        // NOTE: sc_enable_gen_check is always true in debug, but this method requires
        // "hard enabled" generational checks via the template arguments (as it would otherwise fail in release)
        if constexpr (GenCheckEnabled)
        {
            CC_ASSERT(handle != 0 && "accessed null handle");
            internal_handle_t const parsed_handle = cc::bit_cast<internal_handle_t>(handle);
            return (parsed_handle.generation == _generation[parsed_handle.index_plus_one - 1].generation);
        }
        else
        {
            static_assert(GenCheckEnabled, "is_alive requires hard-enabled generational checks");
            return false;
        }
    }

    /// obtain the index of a node
    CC_FORCE_INLINE uint32_t get_node_index(T const* node) const
    {
        CC_ASSERT(node != nullptr);
        CC_ASSERT(node >= &_pool[0] && node < &_pool[_pool_size] && "node outside of pool");
        return uint32_t(node - _pool);
    }

    /// obtain the index of a node
    uint32_t get_handle_index(handle_t handle) const { return _read_handle_index(handle); }

    bool is_full() const { return _first_free_node.load().get_index() == -1; }
    size_t max_size() const { return _pool_size; }

    /// pass a lambda that is called with a T& of each allocated node
    /// acquire CAN be called from within the lambda
    /// release CAN be called from within the lambda ONLY for nodes already iterated (including the current one)
    /// this operation is slow and should not occur in normal operation
    template <class F>
    uint32_t iterate_allocated_nodes(F&& func, cc::allocator* scratch_alloc = cc::system_allocator)
    {
        static_assert(sizeof(T) > 0, "requires complete type");

        if (_pool == nullptr)
            return 0;

        auto const free_indices = _get_free_node_indices(scratch_alloc);

        uint32_t num_iterated_nodes = 0;
        uint32_t free_list_index = 0;
        for (uint32_t i = 0u; i < _pool_size; ++i)
        {
            if (free_list_index >= free_indices.size() || i < free_indices[free_list_index])
            {
                // no free indices left, or before the next free index
                ++num_iterated_nodes;
                T& node = _pool[i];

                if constexpr (std::is_invocable_r_v<bool, F, T&>)
                {
                    // the lambda returns bool, stop iteration if it returns false
                    if (!func(node))
                    {
                        break;
                    }
                }
                else
                {
                    // the lambda returns void
                    func(node);
                }
            }
            else
            {
                // on a free index
                CC_ASSERT(i == free_indices[free_list_index]);
                ++free_list_index;
            }
        }

        return num_iterated_nodes;
    }

    /// This operation is slow and should not occur in normal operation
    uint32_t release_all(cc::allocator* scratch_alloc = cc::system_allocator)
    {
        return iterate_allocated_nodes([this](T& node) { unsafe_release_node(&node); }, scratch_alloc);
    }

    /// release a slot in the pool by the pointer to its node
    /// unsafe: cannot check handle generation
    void unsafe_release_node(T* node)
    {
        static_assert(sizeof(T) > 0, "requires complete type");
        CC_ASSERT(node >= &_pool[0] && node < &_pool[_pool_size] && "node outside of pool");

        uint32_t const real_index = node - _pool;
        if constexpr (sc_enable_gen_check)
        {
            // release not based on handle, so we can't check the generation
            ++_generation[real_index].generation; // increment generation on release
        }

        _release_node(real_index);
    }

    /// returns a valid handle for the index without checking if it is allocated, bypassing future checks
    handle_t unsafe_construct_handle_for_index(uint32_t index) const { return _construct_handle(index); }

public:
    atomic_linked_pool() = default;
    explicit atomic_linked_pool(size_t size, cc::allocator* allocator = cc::system_allocator) { initialize(size, allocator); }
    ~atomic_linked_pool() { _destroy(); }

    atomic_linked_pool(atomic_linked_pool&& rhs) noexcept
      : _pool(rhs._pool),
        _pool_size(rhs._pool_size),
        _first_free_node(cc::move(rhs._first_free_node)),
        _alloc(rhs._alloc),
        _fptr_call_all_dtors(rhs._fptr_call_all_dtors),
        _generation(rhs._generation)
    {
        rhs._pool = nullptr;
    }

    atomic_linked_pool& operator=(atomic_linked_pool&& rhs) noexcept
    {
        _destroy();

        _pool = rhs._pool;
        _pool_size = rhs._pool_size;
        _first_free_node = cc::move(rhs._first_free_node);
        _alloc = rhs._alloc;
        _fptr_call_all_dtors = rhs._fptr_call_all_dtors;
        _generation = rhs._generation;

        rhs._pool = nullptr;
        return *this;
    }

private:
    // internally, generational checks are active in debug even if disabled via template argument
    // explicitly enabling allows for public ::is_alive() functionality
#ifdef CC_ENABLE_ASSERTIONS
    static constexpr bool sc_enable_gen_check = true;
#else
    static constexpr bool sc_enable_gen_check = GenCheckEnabled;
#endif

    enum : size_t
    {
        sc_num_padding_bits = 3,
        sc_num_index_bits = 16,
        sc_num_generation_bits = 32 - (sc_num_padding_bits + sc_num_index_bits),

        // masks non-padding bytes
        // 0b000 <..#sc_num_padding_bits..> 000111 <..rest of uint32..> 111
        sc_padding_mask = ((uint32_t(1) << (32 - sc_num_padding_bits)) - 1),

        // amount of distinct indices that can be stored in the handle
        // -1 because zero is reserved as invalid
        sc_max_size_with_gen_check = (1u << sc_num_index_bits) - 1,
        sc_max_size_without_gen_check = (1u << (32 - sc_num_padding_bits)) - 1
    };

    struct internal_handle_t
    {
        // stores the real index +1 (because 0 is the invalid handle and we never want to hit that)
        uint32_t index_plus_one : sc_num_index_bits; // least significant
        uint32_t generation : sc_num_generation_bits;
        uint32_t padding : sc_num_padding_bits; // most significant
    };
    static_assert(sizeof(internal_handle_t) == sizeof(handle_t));

private:
    // NOTE: Adding these isn't trivial because pointers in the linked list would have to be readjusted
    atomic_linked_pool(atomic_linked_pool const&) = delete;
    atomic_linked_pool& operator=(atomic_linked_pool const&) = delete;

private:
    /// returns indices of unallocated slots, sorted ascending
    cc::alloc_vector<uint32_t> _get_free_node_indices(cc::allocator* scratch_alloc) const
    {
        cc::alloc_vector<uint32_t> free_indices(scratch_alloc);
        free_indices.reserve(_pool_size);

        int32_t cursor = _first_free_node.load(std::memory_order_relaxed).get_index();
        while (cursor != -1)
        {
            free_indices.emplace_back_stable(static_cast<uint32_t>(cursor));

            int32_t* const p_free_list = &_free_list[cursor];
            cursor = (uint32_t)*p_free_list;
        }

        // sort ascending
        auto temp_sortvec = cc::alloc_vector<uint32_t>::uninitialized(free_indices.size(), scratch_alloc);
        detail::radix_sort(free_indices.data(), temp_sortvec.data(), free_indices.size());

        return free_indices;
    }

    handle_t _construct_handle(uint32_t real_index) const
    {
        CC_ASSERT(real_index < _pool_size && "Handle index out of bounds");

        if constexpr (sc_enable_gen_check)
        {
            internal_handle_t res;
            res.padding = 0;
            res.index_plus_one = real_index + 1;
            res.generation = _generation[real_index].generation;
            return cc::bit_cast<uint32_t>(res);
        }
        else
        {
            return real_index + 1;
        }
    }

    CC_FORCE_INLINE uint32_t _read_handle_index(handle_t handle) const
    {
        if constexpr (sc_enable_gen_check)
        {
            CC_ASSERT(handle != 0u && "accessed null handle");
            internal_handle_t const parsed_handle = cc::bit_cast<internal_handle_t>(handle);
            uint32_t const real_index = parsed_handle.index_plus_one - 1;
            CC_ASSERT(real_index < _pool_size && "handle index out of bounds");
            CC_ASSERT(parsed_handle.generation == _generation[real_index].generation && "accessed a stale handle");
            return real_index;
        }
        else
        {
            // we use the handle as-is, but mask out the padding and subtract one
            uint32_t const real_index = (handle & sc_padding_mask) - 1u;
            CC_ASSERT(real_index < _pool_size && "handle index out of bounds");
            return real_index;
        }
    }

    handle_t _read_handle_index_on_release(uint32_t handle) const
    {
        if constexpr (sc_enable_gen_check)
        {
            uint32_t const real_index = _read_handle_index(handle);
            ++_generation[real_index].generation; // increment generation on release
            return real_index;
        }
        else
        {
            return _read_handle_index(handle);
        }
    }

    void _release_node(uint32_t node_idx)
    {
        // call the destructor
        if constexpr (!std::is_trivially_destructible_v<T>)
            _pool[node_idx].~T();

        // to update the free list at this node's index, we need to do another CAS loop
        bool cas_success = false;
        VersionedIndex head_gen_index = _first_free_node.load(std::memory_order_relaxed);
        VersionedIndex new_head_gen_index;
        do
        {
            // the initial load of _first_free_node gave us a _candidate_ for the potential next-pointer to write
            // to our slot of the free list. we write it provisionally, then do a CAS. if we fail, we can safely retry
            // as we still own this node and its slot in the free list

            // store the candidate next-index in our free list slot
            int32_t* const p_free_list = &_free_list[node_idx];
            cc::intrin_atomic_swap(p_free_list, head_gen_index.get_index()); // do a swap to force an atomic store

            // prepare the new value for _first_free_node
            // once again we require a version bump to avoid the ABA problem
            new_head_gen_index = head_gen_index;
            new_head_gen_index.set_index(node_idx);

            // CAS write the newly released index if the expected wasn't raced
            cas_success = _first_free_node.compare_exchange_weak(head_gen_index, new_head_gen_index, std::memory_order_release, std::memory_order_relaxed);

            // if the CAS failed, loop and rewrite the (now different) head_gen_index
        } while (!cas_success);
    }

    void _destroy()
    {
        if (_pool)
        {
            if (_fptr_call_all_dtors)
                _fptr_call_all_dtors(*this);

            _alloc->free(_pool);
            _alloc->free(_free_list);
            _pool = nullptr;
            _pool_size = 0;
            if constexpr (sc_enable_gen_check)
            {
                _alloc->free(_generation);
                _generation = nullptr;
            }
        }
    }

private:
    struct InternalNode
    {
        std::atomic<T*> _next_free;
    };

    // this versioned index is required for our atomic CAS loops
    // to avoid the ABA problem. see more info in acquire() and _release_node()
    // this version is unrelated to the optional _version array
    struct VersionedIndex
    {
        constexpr int32_t get_index() const { return _index; }
        constexpr void set_index(int32_t index)
        {
            _index = index;
            ++_version;
        }

    private:
        int32_t _index = 0;
        uint32_t _version = 0;
    };
    static_assert(std::atomic<VersionedIndex>::is_always_lock_free, "");

    alignas(64) T* _pool = nullptr;

    alignas(64) std::atomic<VersionedIndex> _first_free_node = {};

    alignas(64) int32_t* _free_list = nullptr;

    size_t _pool_size = 0;
    cc::allocator* _alloc = nullptr;

    // function pointer that calls all dtors, used in _destroy() to work with fwd-declared types
    // only non-null if T has a dtor
    void (*_fptr_call_all_dtors)(atomic_linked_pool&) = nullptr;

    // this field is useless for instances without generational checks,
    // but the impact is likely not worth the trouble of conditional inheritance
    internal_handle_t* _generation = nullptr;
};
} // namespace cc
