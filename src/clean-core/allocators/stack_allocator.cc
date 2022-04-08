#include "stack_allocator.hh"

#include <string.h>

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <clean-core/native/memory.hh>

std::byte* cc::stack_allocator::alloc(size_t size, size_t align)
{
    CC_ASSERT(_buffer_begin != nullptr && "stack_allocator uninitialized");

    auto* const padded_res = align_up_with_header(_head, align, sizeof(stack_alloc_header));

    CC_ASSERT(padded_res + size <= _buffer_end && "stack_allocator overcommitted");

    ++_last_alloc_id;
    stack_alloc_header header = {};
    header.padding = uint32_t(padded_res - _head);
    header.alloc_id = _last_alloc_id;

    ::memcpy(padded_res - sizeof(header), &header, sizeof(header));

    _head = padded_res + size;
    return padded_res;
}

void cc::stack_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    CC_ASSERT(alloc_header->alloc_id == _last_alloc_id && "freed ptr was not the most recent allocation");
    --_last_alloc_id;

    _head = byte_ptr - alloc_header->padding;
}

std::byte* cc::stack_allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
{
    if (new_size == 0)
    {
        // free case
        free(ptr);
        return nullptr;
    }
    else if (ptr == nullptr)
    {
        // malloc case
        return this->alloc(new_size, align);
    }

    (void)old_size;
    (void)align;
    // no need to memcpy, the memory remains the same
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    CC_ASSERT(alloc_header->alloc_id == _last_alloc_id && "realloc ptr was not the most recent allocation");
    CC_ASSERT(byte_ptr + new_size <= _buffer_end && "stack_allocator overcommitted");
    CC_ASSERT(old_size == _head - byte_ptr && "incorrect old size");

    _head = byte_ptr + new_size;
    return byte_ptr;
}
