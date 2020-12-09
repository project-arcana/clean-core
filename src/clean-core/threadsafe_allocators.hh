#pragma once

#include <atomic>
#include <mutex>

#include <clean-core/allocator.hh>
#include <clean-core/new.hh>
#include <clean-core/utility.hh>

namespace cc
{
/// thread safe pool allocator, O(1) alloc and free
/// cannot alloc buffers > block_size (ie. does not search contiguous blocks)
/// provided buffer and block size must be aligned to a multiple of all requests (this is verified)
struct atomic_pool_allocator final : allocator
{
    byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        if (size > _block_size)
            return nullptr;

        CC_ASSERT(!is_full() && "pool_allocator full");

        // CAS-loop to acquire a free node and write the matching next pointer
        bool cas_success = false;
        byte* acquired_node = nullptr;
        do
        {
            // acquire-candidate: the current value of _first_free_node
            acquired_node = _first_free_node.load(std::memory_order_acquire);
            // read the in-place next pointer of this node
            byte* const next_pointer_of_acquired = *reinterpret_cast<byte**>(acquired_node);

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

        byte* const freed_node = static_cast<byte*>(ptr);
        CC_ASSERT(freed_node >= _buffer_begin && freed_node - _buffer_begin <= ptrdiff_t(_buffer_size) && "pointer in pool_allocator::free is not part of the buffer");
        CC_ASSERT((freed_node - _buffer_begin) % _block_size == 0 && "freed pointer is not on a node boundary");

        // write the in-place next pointer of this node
        bool cas_success = false;
        do
        {
            byte* expected_first_free = _first_free_node.load(std::memory_order_acquire);

            // write the in-place next pointer of this node provisionally
            new (cc::placement_new, freed_node) byte*(expected_first_free);

            // CAS write the newly released node if the expected wasn't raced
            cas_success = std::atomic_compare_exchange_weak_explicit(&_first_free_node, &expected_first_free, freed_node, std::memory_order_seq_cst,
                                                                     std::memory_order_relaxed);
        } while (!cas_success);
    }

    bool is_full() const { return _first_free_node == nullptr; }
    size_t max_size_bytes() const { return _buffer_size; }
    size_t block_size_bytes() const { return _block_size; }
    size_t max_num_blocks() const { return _buffer_size / _block_size; }

    atomic_pool_allocator() = default;
    atomic_pool_allocator(span<byte> buffer, size_t block_size);

private:
    byte* _buffer_begin = nullptr;
    std::atomic<byte*> _first_free_node = nullptr;
    size_t _buffer_size = 0;
    size_t _block_size = 0;
};

/// thread safe version of cc::linear_allocator
struct atomic_linear_allocator final : allocator
{
    byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        CC_ASSERT(_buffer_begin != nullptr && "atomic_linear_allocator unintialized");

        auto const buffer_size = size + align - 1; // align worst case buffer to satisfy up-aligning
        auto const buffer_start = _offset.fetch_add(buffer_size, std::memory_order_acquire);

        auto* const alloc_start = _buffer_begin + buffer_start;
        auto* const padded_res = cc::align_up(alloc_start, align);

        CC_ASSERT(padded_res - alloc_start < std::ptrdiff_t(align) && "up-align OOB");
        CC_ASSERT(padded_res + size <= _buffer_end && "atomic_linear_allocator overcommitted");

        return padded_res;
    }

    void free(void* ptr) override
    {
        // no-op
        (void)ptr;
    }

    void reset() { _offset.store(0, std::memory_order_release); }

    size_t allocated_size() const { return _offset; }
    size_t max_size() const { return _buffer_end - _buffer_begin; }
    float allocated_ratio() const { return allocated_size() / float(max_size()); }

    atomic_linear_allocator() = default;
    atomic_linear_allocator(span<byte> buffer) : _buffer_begin(buffer.data()), _offset(0), _buffer_end(buffer.data() + buffer.size()) {}

private:
    byte* _buffer_begin = nullptr;
    std::atomic<std::size_t> _offset = {0};
    byte* _buffer_end = nullptr;
};


/// Synchronized (mutexed) version of tlsf_allocator
struct synced_tlsf_allocator final : allocator
{
    synced_tlsf_allocator() = default;
    synced_tlsf_allocator(cc::span<std::byte> buffer) : _backing(buffer) {}
    ~synced_tlsf_allocator() { _backing.destroy(); }


    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = std::lock_guard(_mutex);
        return _backing.alloc(size, align);
    }

    void free(void* ptr) override
    {
        auto lg = std::lock_guard(_mutex);
        _backing.free(ptr);
    }

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = std::lock_guard(_mutex);
        return _backing.realloc(ptr, old_size, new_size, align);
    }

    void initialize(cc::span<std::byte> buffer) { _backing.initialize(buffer); }
    void destroy() { _backing.destroy(); }

private:
    std::mutex _mutex;
    cc::tlsf_allocator _backing;
};
}
