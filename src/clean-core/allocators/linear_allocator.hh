#pragma once

#include <clean-core/allocator.hh>

namespace cc
{
/// trivial linear allocator operating in a given buffer
/// cannot free individual allocations, only reset entirely
/// Will only take as much space as strictly necessary (no allocation headers)
///
struct linear_allocator : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) final override
    {
        CC_ASSERT(_buffer_begin != nullptr && "linear_allocator uninitialized");

        align = cc::max<size_t>(align, 1);

        auto* const padded_res = cc::align_up(_head, align);
        CC_ASSERT(padded_res + size <= _buffer_end && "linear_allocator overcommitted");

        _head = padded_res + size;
        _latest_allocation = padded_res;

        return padded_res;
    }

    void free(void* ptr) final override
    {
        // no-op
        (void)ptr;
    }

    bool get_allocation_size(void const* ptr, size_t& out_size) final override
    {
        if (!ptr || ptr != _latest_allocation)
            return false;

        out_size = _head - _latest_allocation;
        return true;
    }

    std::byte* realloc(void* ptr, size_t new_size, size_t align = alignof(std::max_align_t)) final override
    {
        if (ptr && ptr == _latest_allocation)
        {
            // real realloc
            std::byte* const ptr_byte = static_cast<std::byte*>(ptr);
            CC_ASSERT(ptr_byte + new_size <= _buffer_end && "linear_allocator overcommitted");

            _head = ptr_byte + new_size;
            return ptr_byte;
        }

        // fall back to default behavior
        return cc::allocator::realloc(ptr, new_size, align);
    }

    char const* get_name() const final override { return "Linear Allocator"; }

    void reset()
    {
        _head = _buffer_begin;
        _latest_allocation = nullptr;
    }

    size_t allocated_size() const { return _head - _buffer_begin; }
    size_t remaining_size() const { return _buffer_end - _head; }
    size_t max_size() const { return _buffer_end - _buffer_begin; }
    float allocated_ratio() const { return allocated_size() / float(max_size()); }

    std::byte* buffer() const { return _buffer_begin; }

    linear_allocator() = default;
    explicit linear_allocator(span<std::byte> buffer) : _buffer_begin(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size())
    {
    }

private:
    std::byte* _buffer_begin = nullptr;
    std::byte* _head = nullptr;
    std::byte* _buffer_end = nullptr;
    std::byte* _latest_allocation = nullptr;
};

template <size_t NumBytes>
struct fixed_linear_allocator : linear_allocator
{
    fixed_linear_allocator() : linear_allocator(_memory) {}

private:
    std::byte _memory[NumBytes];
};
} // namespace cc