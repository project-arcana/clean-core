#pragma once

#include <atomic>

#include <clean-core/allocator.hh>

namespace cc
{
/// thread safe version of cc::linear_allocator
/// Can also realloc any allocation
/// Will take up more space than strictly necessary in the given buffer
struct atomic_linear_allocator final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        CC_ASSERT(_buffer_begin != nullptr && "atomic_linear_allocator unintialized");

        align = cc::max<size_t>(align, 1);

        auto const buffer_size = size + (align - 1) + sizeof(size_t); // align worst case buffer to satisfy up-aligning, add header field for alloc size
        auto const buffer_start = _offset.fetch_add(buffer_size, std::memory_order_acquire);

        auto* const alloc_start = _buffer_begin + buffer_start;
        auto* const padded_res = cc::align_up(alloc_start + sizeof(size_t), align);

        // store alloc size
        *((size_t*)(padded_res - sizeof(size_t))) = size;

        CC_ASSERT(padded_res - (alloc_start + sizeof(size_t)) < std::ptrdiff_t(align) && "up-align OOB");
        CC_ASSERT(padded_res + size <= _buffer_end && "atomic_linear_allocator overcommitted");

        return padded_res;
    }

    void free(void* ptr) override
    {
        // no-op
        (void)ptr;
    }

    bool get_allocation_size(void const* ptr, size_t& out_size) override
    {
        if (!ptr)
            return false;

        out_size = *((size_t const*)((std::byte const*)ptr - sizeof(size_t)));
        return true;
    }

    char const* get_name() const override { return "Atomic Linear Allocator"; }

    void reset() { _offset.store(0, std::memory_order_release); }

    size_t allocated_size() const { return _offset.load(); }
    size_t max_size() const { return _buffer_end - _buffer_begin; }
    float allocated_ratio() const { return allocated_size() / float(max_size()); }

    atomic_linear_allocator() = default;
    explicit atomic_linear_allocator(span<std::byte> buffer) : _buffer_begin(buffer.data()), _offset(0), _buffer_end(buffer.data() + buffer.size()) {}

    void initialize(span<std::byte> buffer)
    {
        // atomics cant be moved, making this necessary
        _buffer_begin = buffer.data();
        _offset = 0;
        _buffer_end = buffer.data() + buffer.size();
    }

private:
    std::byte* _buffer_begin = nullptr;
    std::atomic<std::size_t> _offset = {0};
    std::byte* _buffer_end = nullptr;
};
}