#pragma once

#include <atomic>

#include <clean-core/allocator.hh>

namespace cc
{
/// thread safe pool allocator, O(1) alloc and free
/// cannot alloc buffers > block_size (ie. does not search contiguous blocks)
/// provided buffer and block size must be aligned to a multiple of all requests (this is verified)
struct atomic_pool_allocator final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        if (size > _block_size)
            return nullptr;

        CC_ASSERT(!is_full() && "pool_allocator full");

        // CAS-loop to acquire a free node and write the matching next pointer
        bool cas_success = false;
        std::byte* acquired_node = nullptr;
        do
        {
            // acquire-candidate: the current value of _first_free_node
            acquired_node = _first_free_node.load(std::memory_order_acquire);
            // read the in-place next pointer of this node
            std::byte* const next_pointer_of_acquired = *reinterpret_cast<std::byte**>(acquired_node);

            // compare-exchange these two - spurious failure if raced
            cas_success = std::atomic_compare_exchange_weak_explicit(&_first_free_node, &acquired_node, next_pointer_of_acquired,
                                                                     std::memory_order_seq_cst, std::memory_order_relaxed);
        } while (!cas_success);

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

        // write the in-place next pointer of this node
        bool cas_success = false;
        do
        {
            std::byte* expected_first_free = _first_free_node.load(std::memory_order_acquire);

            // write the in-place next pointer of this node provisionally
            new (cc::placement_new, freed_node) std::byte*(expected_first_free);

            // CAS write the newly released node if the expected wasn't raced
            cas_success = std::atomic_compare_exchange_weak_explicit(&_first_free_node, &expected_first_free, freed_node, std::memory_order_seq_cst,
                                                                     std::memory_order_relaxed);
        } while (!cas_success);
    }

    /// returns the offset of a node to the buffer start in bytes
    size_t get_node_offset_bytes(void const* ptr) const
    {
        std::byte const* const node = static_cast<std::byte const*>(ptr);
        CC_ASSERT(node >= _buffer_begin && node - _buffer_begin <= ptrdiff_t(_buffer_size)
                  && "pointer in pool_allocator::get_node_offset_bytes is not part of the buffer");
        CC_ASSERT((node - _buffer_begin) % _block_size == 0 && "pointer is not on a node boundary");
        return size_t(node - _buffer_begin);
    }

    size_t get_node_index(void const* ptr) const
    {
        auto const offset_bytes = get_node_offset_bytes(ptr);
        return offset_bytes / _block_size;
    }

    bool is_full() const { return _first_free_node == nullptr; }
    size_t max_size_bytes() const { return _buffer_size; }
    size_t block_size_bytes() const { return _block_size; }
    size_t max_num_blocks() const { return _buffer_size / _block_size; }

    atomic_pool_allocator() = default;
    explicit atomic_pool_allocator(span<std::byte> buffer, size_t block_size);

    void initialize(span<std::byte> buffer, size_t block_size);

    template <class T>
    void initialize_from_array(span<T> elements, size_t block_size_in_elements = 1)
    {
        return initialize(cc::as_byte_span(elements), sizeof(T) * block_size_in_elements);
    }

private:
    std::byte* _buffer_begin = nullptr;
    std::atomic<std::byte*> _first_free_node = nullptr;
    size_t _buffer_size = 0;
    size_t _block_size = 0;
};

}