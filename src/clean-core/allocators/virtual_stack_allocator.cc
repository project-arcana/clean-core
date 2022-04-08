#include "virtual_stack_allocator.hh"

#include <clean-core/bits.hh>
#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#include <clean-core/native/memory.hh>


void cc::virtual_stack_allocator::initialize(size_t max_size_bytes, size_t chunk_size_bytes)
{
    _virtual_begin = reserve_virtual_memory(max_size_bytes);
    _virtual_end = _virtual_begin + max_size_bytes;
    _physical_current = _virtual_begin;
    _physical_end = _virtual_begin;
    _last_alloc_id = 0;
    _chunk_size_bytes = chunk_size_bytes;

    CC_ASSERT(max_size_bytes > 0 && chunk_size_bytes > 0 && "invalid sizes");
    CC_ASSERT(is_pow2(chunk_size_bytes) && "Chunk size must be a power of 2");
    CC_ASSERT(_virtual_begin != nullptr && "virtual reserve failed");
}

void cc::virtual_stack_allocator::destroy()
{
    if (_virtual_begin)
    {
        free_virtual_memory(_virtual_begin, _virtual_end - _virtual_begin);
        _virtual_begin = nullptr;
    }
}

std::byte* cc::virtual_stack_allocator::alloc(size_t size, size_t align)
{
    CC_ASSERT(_virtual_begin != nullptr && "virtual_stack_allocator uninitialized");

    std::byte* const padded_res = align_up_with_header(_physical_current, align, sizeof(stack_alloc_header));
    size_t const num_padding_bytes = size_t(padded_res - _physical_current);
    size_t const required_size = num_padding_bytes + size;

    _physical_end = grow_physical_memory(_physical_current, _physical_end, _virtual_end, _chunk_size_bytes, required_size);

    ++_last_alloc_id;
    stack_alloc_header header = {};
    header.padding = uint32_t(num_padding_bytes);
    header.alloc_id = _last_alloc_id;

    std::memcpy(padded_res - sizeof(header), &header, sizeof(header));

    _physical_current = padded_res + size;
    return padded_res;
}

void cc::virtual_stack_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    CC_ASSERT(alloc_header->alloc_id == _last_alloc_id && "freed ptr was not the most recent allocation");
    --_last_alloc_id;
    _physical_current = byte_ptr - alloc_header->padding;
}

std::byte* cc::virtual_stack_allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
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

    // no need to memcpy, the memory remains the same
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    CC_ASSERT(alloc_header->alloc_id == _last_alloc_id && "realloc ptr was not the most recent allocation");
    CC_ASSERT(old_size == _physical_current - byte_ptr && "incorrect old size");

    if (new_size > old_size)
    {
        // grow physically to meet demand
        size_t const num_new_bytes = new_size - old_size;
        _physical_end = grow_physical_memory(_physical_current, _physical_end, _virtual_end, _chunk_size_bytes, num_new_bytes);
    }

    _physical_current = byte_ptr + new_size;
    return byte_ptr;
}

size_t cc::virtual_stack_allocator::reset()
{
    size_t const num_bytes_allocated = _physical_current - _virtual_begin;
    _physical_current = _virtual_begin;
    _last_alloc_id = 0;
    return num_bytes_allocated;
}

size_t cc::virtual_stack_allocator::decommit_idle_memory()
{
    // align up to the start of the first empty page
    std::byte* const ptr = cc::align_up(_physical_current, _chunk_size_bytes);
    // then free all memory between that and _physical_end
    ptrdiff_t const size_to_free = _physical_end - ptr;

    if (size_to_free > 0)
    {
        decommit_physical_memory(ptr, size_t(size_to_free));
        _physical_end = ptr;
    }

    return size_to_free;
}

bool cc::virtual_stack_allocator::is_latest_allocation(void* ptr) const
{
    CC_CONTRACT(ptr);

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    CC_ASSERT(byte_ptr > _virtual_begin && _physical_end && "pointer not inside physical region");

    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));
    return alloc_header->alloc_id == _last_alloc_id;
}
