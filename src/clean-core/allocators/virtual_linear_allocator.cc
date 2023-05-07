#include "virtual_linear_allocator.hh"

#include <clean-core/bits.hh>
#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#include <clean-core/native/memory.hh>

void cc::virtual_linear_allocator::initialize(size_t max_size_bytes, size_t chunk_size_bytes)
{
    _virtual_begin = reserve_virtual_memory(max_size_bytes);
    _virtual_end = _virtual_begin + max_size_bytes;
    _physical_current = _virtual_begin;
    _physical_end = _virtual_begin;
    _chunk_size_bytes = chunk_size_bytes;

    CC_ASSERT(max_size_bytes > 0 && chunk_size_bytes > 0 && "invalid sizes");
    CC_ASSERT(is_pow2(chunk_size_bytes) && "Chunk size must be a power of 2");
    CC_ASSERT(_virtual_begin != nullptr && "virtual reserve failed");
}

void cc::virtual_linear_allocator::destroy()
{
    if (_virtual_begin)
    {
        free_virtual_memory(_virtual_begin, _virtual_end - _virtual_begin);
        _virtual_begin = nullptr;
    }
}

std::byte* cc::virtual_linear_allocator::alloc(size_t size, size_t align)
{
    CC_ASSERT(_virtual_begin != nullptr && "virtual_linear_allocator uninitialized");

    std::byte* const padded_res = cc::align_up(_physical_current + sizeof(size_t), align);
    size_t const num_padding_bytes = size_t(padded_res - _physical_current);
    size_t const required_size = num_padding_bytes + size;

    _physical_end = grow_physical_memory(_physical_current, _physical_end, _virtual_end, _chunk_size_bytes, required_size);

    _physical_current = padded_res + size;
    _last_allocation = padded_res;

    // store alloc size
    *((size_t*)(padded_res - sizeof(size_t))) = size;

    return padded_res;
}

bool cc::virtual_linear_allocator::get_allocation_size(void const* ptr, size_t& out_size)
{
    if (!ptr)
        return false;

    out_size = *((size_t const*)((std::byte const*)ptr - sizeof(size_t)));
    return true;
}

std::byte* cc::virtual_linear_allocator::realloc(void* ptr, size_t new_size, size_t align)
{
    if (!ptr || ptr != _last_allocation)
    {
        // cannot realloc, fall back (this is not invalid usage unlike for stack_linear_allocator)
        return cc::allocator::realloc(ptr, new_size, align);
    }

    // true realloc
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);

    size_t old_size;
    bool const success = get_allocation_size(ptr, old_size);
    CC_ASSERT(success && old_size == _physical_current - byte_ptr && "incorrect old size");

    if (new_size > old_size)
    {
        // grow physically to meet demand
        size_t const num_new_bytes = new_size - old_size;
        _physical_end = grow_physical_memory(_physical_current, _physical_end, _virtual_end, _chunk_size_bytes, num_new_bytes);
    }

    // store new alloc size
    *((size_t*)(byte_ptr - sizeof(size_t))) = new_size;

    _physical_current = byte_ptr + new_size;
    return byte_ptr;
}

size_t cc::virtual_linear_allocator::decommit_idle_memory()
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
