#pragma once

#include <cstdint>
#include <cstring>

#include <atomic>

#include <clean-core/alloc_vector.hh>
#include <clean-core/allocator.hh>
#include <clean-core/bit_cast.hh>
#include <clean-core/bits.hh>
#include <clean-core/new.hh>

namespace cc
{
namespace detail
{
void radix_sort(uint32_t* __restrict a, uint32_t* __restrict temp, size_t n);
}

/// Fixed-size object pool, synchronized and lock-free
/// Uses an in-place linked list in free nodes, for O(1) acquire, release and size overhead
/// Pointers remain stable
/// acquire() and release() fully thread-safe
/// (access to underlying memory unsynchronized)
template <class T, bool GenCheckEnabled>
struct atomic_linked_pool
{
    using handle_t = uint32_t;

    void initialize(size_t size, cc::allocator* allocator = cc::system_allocator)
    {
        static_assert(sizeof(T) >= sizeof(T*), "linked_pool element type must be large enough to accomodate a pointer");
        if (size == 0)
            return;

        if constexpr (sc_enable_gen_check)
        {
            CC_ASSERT(size < 1u << sc_num_index_bits && "linked_pool size too large for index type");
        }
        else
        {
            CC_ASSERT(size < (1u << (32 - sc_num_padding_bits)) && "linked_pool size too large for index type");
        }

        CC_ASSERT(_pool == nullptr && "re-initialized linked_pool");
        CC_CONTRACT(allocator != nullptr);

        _alloc = allocator;

        _pool_size = size;
        _pool = reinterpret_cast<T*>(_alloc->alloc(sizeof(T) * _pool_size, alignof(T)));

        // initialize linked list
        for (auto i = 0u; i < _pool_size - 1; ++i)
        {
            T* node_ptr = &_pool[i];
            new (cc::placement_new, node_ptr) T*(&_pool[i + 1]);
        }

        // initialize linked list tail
        {
            T* tail_ptr = &_pool[_pool_size - 1];
            new (cc::placement_new, tail_ptr) T*(nullptr);
        }

        if constexpr (sc_enable_gen_check)
        {
            // initialize generation handles
            _generation = reinterpret_cast<internal_handle_t*>(_alloc->alloc(sizeof(internal_handle_t) * _pool_size, alignof(internal_handle_t)));
            std::memset(_generation, 0, sizeof(internal_handle_t) * _pool_size);
        }

        _first_free_node = &_pool[0];
    }

    void destroy() { _destroy(); }

    /// acquire a new slot in the pool
    [[nodiscard]] handle_t acquire()
    {
        CC_ASSERT(!is_full() && "linked_pool full");

        // CAS-loop to acquire a free node and write the matching next pointer
        bool cas_success = false;
        T* acquired_node = nullptr;
        do
        {
            // acquire-candidate: the current value of _first_free_node
            acquired_node = _first_free_node.load(std::memory_order_acquire);
            // read the in-place next pointer of this node
            T* const next_pointer_of_acquired = *reinterpret_cast<T**>(acquired_node);

            // compare-exchange these two - spurious failure if raced
            cas_success = std::atomic_compare_exchange_weak_explicit(&_first_free_node, &acquired_node, next_pointer_of_acquired,
                                                                     std::memory_order_seq_cst, std::memory_order_relaxed);
        } while (!cas_success);

        // call the constructor
        if constexpr (!std::is_trivially_constructible_v<T>)
            new (cc::placement_new, acquired_node) T();

        // calculate the index
        uint32_t const res_index = uint32_t(acquired_node - _pool);

        // construct a handle
        return _construct_handle(res_index);
    }

    /// release a slot in the pool, destroying it
    void release(handle_t handle)
    {
        uint32_t real_index = _read_handle_index_on_release(handle);

        T* const released_node = &_pool[real_index];
        _release_node(released_node);
    }

    /// access a slot
    CC_FORCE_INLINE T& get(handle_t handle)
    {
        uint32_t index = _read_handle_index(handle);
        CC_CONTRACT(index < _pool_size);
        return _pool[index];
    }

    /// access a slot
    CC_FORCE_INLINE T const& get(handle_t handle) const
    {
        uint32_t index = _read_handle_index(handle);
        CC_CONTRACT(index < _pool_size);
        return _pool[index];
    }

    /// test if a handle references a currently live slot,
    /// requires GenCheckEnabled (template parameter)
    bool is_alive(handle_t handle) const
    {
        // NOTE: sc_enable_gen_check is always true in debug, but this method requires
        // "hard enabled" generational checks via the template arguments (as it would otherwise fail in release)
        if constexpr (GenCheckEnabled)
        {
            CC_ASSERT(handle != uint32_t(-1) && "accessed null handle");
            internal_handle_t const parsed_handle = cc::bit_cast<internal_handle_t>(handle);
            return (parsed_handle.generation == _generation[parsed_handle.index].generation);
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
        CC_ASSERT(node >= &_pool[0] && node < &_pool[_pool_size] && "node outside of pool");
        return node - _pool;
    }

    /// obtain the index of a node
    uint32_t get_handle_index(handle_t handle) const { return _read_handle_index(handle); }

    bool is_full() const { return _first_free_node == nullptr; }
    size_t max_size() const { return _pool_size; }

    /// pass a lambda that is called with a T& of each allocated node
    /// acquire CAN be called from within the lambda
    /// release CAN be called from within the lambda ONLY for nodes already iterated (including the current one)
    /// this operation is slow and should not occur in normal operation
    template <class F>
    unsigned iterate_allocated_nodes(F&& func, cc::allocator* scratch_alloc = cc::system_allocator)
    {
        if (_pool == nullptr)
            return 0;

        auto const free_indices = _get_free_node_indices(scratch_alloc);

        unsigned num_iterated_nodes = 0;
        unsigned free_list_index = 0;
        for (handle_t i = 0u; i < _pool_size; ++i)
        {
            if (free_list_index >= free_indices.size() || i < free_indices[free_list_index])
            {
                // no free indices left, or before the next free index
                func(_pool[i]);
                ++num_iterated_nodes;
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
    unsigned release_all(cc::allocator* scratch_alloc = cc::system_allocator)
    {
        return iterate_allocated_nodes([this](T& node) { unsafe_release_node(&node); }, scratch_alloc);
    }

    /// release a slot in the pool by the pointer to it's node
    /// unsafe: cannot check handle generation
    void unsafe_release_node(T* node)
    {
        CC_ASSERT(node >= &_pool[0] && node < &_pool[_pool_size] && "node outside of pool");

        if constexpr (sc_enable_gen_check)
        {
            // release not based on handle, so we can't check the generation
            ++_generation[node - _pool].generation; // increment generation on release
        }

        _release_node(node);
    }

    /// returns a valid handle for the index without checking if it is allocated, bypassing future checks
    handle_t unsafe_construct_handle_for_index(uint32_t index) const { return _construct_handle(index); }

public:
    atomic_linked_pool() = default;
    explicit atomic_linked_pool(size_t size, cc::allocator* allocator = cc::system_allocator) { initialize(size, allocator); }
    ~atomic_linked_pool() { _destroy(); }

    atomic_linked_pool(atomic_linked_pool&& rhs) noexcept
      : _pool(rhs._pool), _pool_size(rhs._pool_size), _first_free_node(cc::move(rhs._first_free_node)), _alloc(rhs._alloc), _generation(rhs._generation)
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
        sc_num_generation_bits = 32 - (sc_num_padding_bits + sc_num_index_bits)
    };

    struct internal_handle_t
    {
        uint32_t index : sc_num_index_bits; // least significant
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
    cc::alloc_vector<handle_t> _get_free_node_indices(cc::allocator* scratch_alloc) const
    {
        cc::alloc_vector<handle_t> free_indices(scratch_alloc);
        free_indices.reserve(_pool_size);

        T* cursor = _first_free_node;
        while (cursor != nullptr)
        {
            free_indices.push_back(static_cast<handle_t>(cursor - _pool));
            cursor = *reinterpret_cast<T**>(cursor);
        }
        // sort ascending
        auto temp_sortvec = cc::alloc_vector<handle_t>::uninitialized(free_indices.size(), scratch_alloc);
        detail::radix_sort(free_indices.data(), temp_sortvec.data(), free_indices.size());

        return free_indices;
    }

    handle_t _construct_handle(uint32_t real_index) const
    {
        if constexpr (sc_enable_gen_check)
        {
            internal_handle_t res;
            res.padding = 0;
            res.index = real_index;
            res.generation = _generation[real_index].generation;
            return cc::bit_cast<uint32_t>(res);
        }
        else
        {
            return real_index;
        }
    }

    CC_FORCE_INLINE uint32_t _read_handle_index(handle_t handle) const
    {
        if constexpr (sc_enable_gen_check)
        {
            CC_ASSERT(handle != uint32_t(-1) && "accessed null handle");
            internal_handle_t const parsed_handle = cc::bit_cast<internal_handle_t>(handle);
            uint32_t const real_index = parsed_handle.index;
            CC_ASSERT(parsed_handle.generation == _generation[real_index].generation && "accessed a stale handle");
            return real_index;
        }
        else
        {
            // we use the handle as-is, but mask out the padding
            // mask is
            // 0b000 <..#sc_num_padding_bits..> 000111 <..rest of uint32..> 111
            enum : uint32_t
            {
                e_padding_mask = ((uint32_t(1) << (32 - sc_num_padding_bits)) - 1)
            };
            return handle & e_padding_mask;
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

    void _release_node(T* released_node)
    {
        // call the destructor
        if constexpr (!std::is_trivially_destructible_v<T>)
            released_node->~T();

        // write the in-place next pointer of this node
        bool cas_success = false;
        do
        {
            T* expected_first_free = _first_free_node.load(std::memory_order_acquire);

            // write the in-place next pointer of this node provisionally
            new (cc::placement_new, released_node) T*(expected_first_free);

            // CAS write the newly released node if the expected wasn't raced
            cas_success = std::atomic_compare_exchange_weak_explicit(&_first_free_node, &expected_first_free, released_node,
                                                                     std::memory_order_seq_cst, std::memory_order_relaxed);
        } while (!cas_success);
    }

    void _destroy()
    {
        if (_pool)
        {
            _alloc->free(_pool);
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
    T* _pool = nullptr;
    size_t _pool_size = 0;

    std::atomic<T*> _first_free_node = nullptr;
    cc::allocator* _alloc = nullptr;

    // this field is useless for instances without generational checks,
    // but the impact is likely not worth the trouble of conditional inheritance
    internal_handle_t* _generation = nullptr;
};
}
