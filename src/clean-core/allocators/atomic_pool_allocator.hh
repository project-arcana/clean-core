#pragma once

#include <atomic>

#include <clean-core/allocator.hh>
#include <clean-core/intrinsics.hh>

namespace cc
{
/// thread safe pool allocator, O(1) alloc and free
/// cannot alloc buffers > block_size (ie. does not search contiguous blocks)
/// provided buffer and block size must be aligned to a multiple of all requests (this is verified)
/// RESTRICTION: Must only allocate buffers with size <= block_size set at init
struct atomic_pool_allocator final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        CC_ASSERT(!is_full() && "pool_allocator full");
        CC_ASSERT(size <= _block_size && "Can only allocate buffers up to the block size");

        // CAS-loop to acquire a free node and write the matching next pointer
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

        CC_ASSERT(acquired_node_index >= 0 && acquired_node_index < max_num_blocks());
        std::byte* const acquired_node = _buffer_begin + (size_t(acquired_node_index) * _block_size);

        CC_ASSERT(cc::is_aligned(acquired_node, align) && "pool buffer and blocks must be aligned to a multiple of all requests");
        return acquired_node;
    }

    void free(void* ptr) override
    {
        if (!ptr)
            return;

        std::byte* const freed_node = static_cast<std::byte*>(ptr);
        CC_ASSERT(freed_node >= _buffer_begin && freed_node - _buffer_begin <= ptrdiff_t(_buffer_size) && "pointer in pool_allocator::free is not part of the buffer");
        CC_ASSERT((freed_node - _buffer_begin) % _block_size == 0 && "freed pointer is not on a node boundary");

        uint32_t const freed_node_index = uint32_t((freed_node - _buffer_begin) / _block_size);

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
            int32_t* const p_free_list = &_free_list[freed_node_index];
            cc::intrin_atomic_swap(p_free_list, head_gen_index.get_index()); // do a swap to force an atomic store

            // prepare the new value for _first_free_node
            // once again we require a version bump to avoid the ABA problem
            new_head_gen_index = head_gen_index;
            new_head_gen_index.set_index(freed_node_index);

            // CAS write the newly released index if the expected wasn't raced
            cas_success = _first_free_node.compare_exchange_weak(head_gen_index, new_head_gen_index, std::memory_order_release, std::memory_order_relaxed);

            // if the CAS failed, loop and rewrite the (now different) head_gen_index
        } while (!cas_success);
    }

    bool get_allocation_size(void const* ptr, size_t& out_size) override
    {
        if (!ptr)
            return false;

        out_size = _block_size;
        return true;
    }

    char const* get_name() const override { return "Atomic Pool Allocator"; }

    /// returns the offset of a node to the buffer start in bytes
    CC_FORCE_INLINE size_t get_node_offset_bytes(void const* ptr) const
    {
        std::byte const* const node = static_cast<std::byte const*>(ptr);
        CC_ASSERT(node >= _buffer_begin && node - _buffer_begin <= ptrdiff_t(_buffer_size)
                  && "pointer in pool_allocator::get_node_offset_bytes is not part of the buffer");
        CC_ASSERT((node - _buffer_begin) % _block_size == 0 && "pointer is not on a node boundary");
        return size_t(node - _buffer_begin);
    }

    CC_FORCE_INLINE size_t get_node_index(void const* ptr) const
    {
        auto const offset_bytes = get_node_offset_bytes(ptr);
        return offset_bytes / _block_size;
    }

    CC_FORCE_INLINE bool is_full() const { return _first_free_node.load().get_index() == -1; }
    CC_FORCE_INLINE size_t max_size_bytes() const { return _buffer_size; }
    CC_FORCE_INLINE size_t block_size_bytes() const { return _block_size; }
    CC_FORCE_INLINE size_t max_num_blocks() const { return _buffer_size / _block_size; }

    cc::span<std::byte> get_buffer() { return cc::span{_buffer_begin, _buffer_size}; }
    cc::span<std::byte const> get_buffer() const { return cc::span{_buffer_begin, _buffer_size}; }

    atomic_pool_allocator() = default;
    explicit atomic_pool_allocator(cc::allocator* alloc, size_t block_size_bytes, size_t num_blocks, size_t buffer_align = 8);
    ~atomic_pool_allocator() { destroy(); }

    // initialize the pool allocator
    // block_size_bytes: how large individual allocations are, in bytes
    // num_blocks: amount of blocks - also max. amount of live allocations at once
    // buffer_align: the alignment of the entire backing buffer
    void initialize(cc::allocator* alloc, size_t block_size_bytes, size_t num_blocks, size_t buffer_align = 8);
    void destroy();

    // initialize the pool allocator, conceptually representing a large array of Ts, divided into blocks
    // T: the type of the underlying array
    // num_blocks: the amount of subdivisions - also max. amount of live allocations at once
    // num_elements_per_block: the size of individual blocks in elements (Ts)
    template <class T>
    void initialize_as_array_pool(cc::allocator* alloc, size_t num_blocks, size_t num_elements_per_block = 1)
    {
        initialize(alloc, sizeof(T) * num_elements_per_block, num_blocks, alignof(T));
    }

private:
    // this versioned index is required for our atomic CAS loops
    // to avoid the ABA problem. see more info in alloc() and free()
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

private:
    alignas(64) std::byte* _buffer_begin = nullptr;
    alignas(64) std::atomic<VersionedIndex> _first_free_node = {};
    alignas(64) int32_t* _free_list = nullptr;

    size_t _buffer_size = 0;
    size_t _block_size = 0;
    cc::allocator* _backing_alloc = nullptr;
};

} // namespace cc