#pragma once

#include <clean-core/allocator.hh>

namespace cc
{
/// stack allocator operating in a given buffer
/// like a linear allocator, but can also free the most recent allocation
struct stack_allocator final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    /// NOTE: ptr must be the most recent allocation received
    void free(void* ptr) override;

    /// NOTE: ptr must be the most recent allocation received
    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    void reset()
    {
        _head = _buffer_begin;
        _last_alloc_id = 0;
    }

    stack_allocator() = default;
    explicit stack_allocator(span<std::byte> buffer) : _buffer_begin(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size())
    {
    }

private:
    std::byte* _buffer_begin = nullptr;
    std::byte* _head = nullptr;
    std::byte* _buffer_end = nullptr;
    int32_t _last_alloc_id = 0;
};
}