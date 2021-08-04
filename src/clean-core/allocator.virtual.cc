#include "allocator.hh"

#include <clean-core/bits.hh>
#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#include <clean-core/native/memory.hh>

namespace
{
struct vstack_alloc_header
{
    uint32_t padding;
    int32_t alloc_id;
};

// [... pad ...] [header] [data]
std::byte* valign_up_with_header(std::byte* head, size_t align, size_t header_size)
{
    std::byte* padded_res = cc::align_up(head, align);
    size_t const padding = size_t(padded_res - head);

    if (padding < header_size)
    {
        // header does not fit - align up
        header_size -= padding;

        if (header_size % align > 0)
        {
            padded_res += align * (1 + (header_size / align));
        }
        else
        {
            padded_res += align * (header_size / align);
        }
    }

    return padded_res;
}

std::byte* grow_physical_memory(std::byte* physical_current, std::byte* physical_end, std::byte* virtual_end, size_t chunk_size, size_t grow_num_bytes)
{
    if (physical_current + grow_num_bytes <= physical_end)
        return physical_end; // no growth required

    // round up new size to multiple of chunk size
    size_t const new_commit_size = cc::align_up(grow_num_bytes, chunk_size);
    CC_ASSERT(physical_end + new_commit_size <= virtual_end && "virtual_linear_allocator overcommitted");

    // allocate new pages at the end of current physical commit range
    cc::commit_physical_memory(physical_end, new_commit_size);
    physical_end += new_commit_size;
    return physical_end;
}
}

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

    std::byte* const padded_res = cc::align_up(_physical_current, align);
    size_t const num_padding_bytes = size_t(padded_res - _physical_current);
    size_t const required_size = num_padding_bytes + size;

    _physical_end = grow_physical_memory(_physical_current, _physical_end, _virtual_end, _chunk_size_bytes, required_size);

    _physical_current = padded_res + size;
    _last_allocation = padded_res;
    return padded_res;
}

std::byte* cc::virtual_linear_allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
{
    if (!ptr || ptr != _last_allocation)
    {
        return new_size > 0 ? this->alloc(new_size, align) : nullptr;
    }

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
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

size_t cc::virtual_linear_allocator::decommit_idle_memory()
{
    // align up to the start of the first empty page
    std::byte* const ptr = cc::align_up(_physical_current, _chunk_size_bytes);
    // then free all memory between that and _physical_end
    size_t const size_to_free = _physical_end - ptr;

    if (size_to_free > 0)
    {
        decommit_physical_memory(ptr, size_to_free);
        _physical_end = ptr;
    }

    return size_to_free;
}

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

    std::byte* const padded_res = valign_up_with_header(_physical_current, align, sizeof(vstack_alloc_header));
    size_t const num_padding_bytes = size_t(padded_res - _physical_current);
    size_t const required_size = num_padding_bytes + size;

    _physical_end = grow_physical_memory(_physical_current, _physical_end, _virtual_end, _chunk_size_bytes, required_size);

    ++_last_alloc_id;
    vstack_alloc_header header = {};
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
    vstack_alloc_header const* const alloc_header = (vstack_alloc_header*)(byte_ptr - sizeof(vstack_alloc_header));

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

    (void)align;

    // no need to memcpy, the memory remains the same
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    vstack_alloc_header const* const alloc_header = (vstack_alloc_header*)(byte_ptr - sizeof(vstack_alloc_header));

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
    size_t const size_to_free = _physical_end - ptr;

    if (size_to_free > 0)
    {
        decommit_physical_memory(ptr, size_to_free);
        _physical_end = ptr;
    }

    return size_to_free;
}

bool cc::virtual_stack_allocator::is_latest_allocation(void* ptr) const
{
    CC_CONTRACT(ptr);

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    CC_ASSERT(byte_ptr > _virtual_begin && _physical_end && "pointer not inside physical region");

    vstack_alloc_header const* const alloc_header = (vstack_alloc_header*)(byte_ptr - sizeof(vstack_alloc_header));
    return alloc_header->alloc_id == _last_alloc_id;
}
