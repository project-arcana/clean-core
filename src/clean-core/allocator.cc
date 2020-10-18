#include <clean-core/allocator.hh>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

// enable debug trace printfs in some allocators
#define CC_DEBUG_TRACE_SCRATCH_ALLOC 0

#if CC_DEBUG_TRACE_SCRATCH_ALLOC
#define CC_DTRACE_ALLOC_PRINTF printf
#define CC_DTRACE_ALLOC_FFLUSH() fflush(stdout)
#else
#define CC_DTRACE_ALLOC_PRINTF(...) (void)0
#define CC_DTRACE_ALLOC_FFLUSH() (void)0
#endif

namespace
{
struct stack_alloc_header
{
    size_t padding;
};

struct scratch_alloc_header
{
    uint32_t size;
};

constexpr uint32_t const gc_header_pad_value = 0xffffffffu;
constexpr uint32_t const gc_header_free_bit = 0x80000000u;

// [... pad ...] [header] [data]
std::byte* align_up_with_header(std::byte* head, size_t align, size_t header_size)
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

// [header] [... pad ...] [data]
std::byte* align_up_from_header(scratch_alloc_header* header, size_t align)
{
    void* const p = header + 1;
    return static_cast<std::byte*>(cc::align_up(p, align));
}

// returns a pointer to the header preceding the given allocation
scratch_alloc_header* get_header_before_pointer(void* ptr)
{
    uint32_t* p = (uint32_t*)ptr;
    while (p[-1] == gc_header_pad_value)
        --p;
    return (scratch_alloc_header*)p - 1;
}

// writes the size of an allocation to a header, and 0xFF to the bytes between the header and the allocation
void fill_in_header_value(scratch_alloc_header* header, void* associated_data_ptr, uint32_t size)
{
    header->size = size;
    uint32_t* p = (uint32_t*)(header + 1);
    while (p < associated_data_ptr)
        *p++ = gc_header_pad_value;
}
}

cc::byte* cc::linear_allocator::alloc(cc::size_t size, cc::size_t align)
{
    CC_ASSERT(_buffer_begin != nullptr && "linear_allocator uninitialized");

    auto* const padded_res = align_up(_head, align);

    CC_ASSERT(padded_res + size <= _buffer_end && "linear_allocator overcommitted");

    _head = padded_res + size;
    return padded_res;
}

cc::byte* cc::stack_allocator::alloc(cc::size_t size, cc::size_t align)
{
    CC_ASSERT(_buffer_begin != nullptr && "stack_allocator uninitialized");

    auto* const padded_res = align_up_with_header(_head, align, sizeof(stack_alloc_header));

    CC_ASSERT(padded_res + size <= _buffer_end && "stack_allocator overcommitted");

    stack_alloc_header header = {};
    header.padding = size_t(padded_res - _head);

    std::memcpy(padded_res - sizeof(header), &header, sizeof(header));

    _head = padded_res + size;
    _last_alloc = padded_res;
    return padded_res;
}

void cc::stack_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;

    CC_ASSERT(ptr == _last_alloc && "argument to stack_allocator::free was not the most recent allocation");

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    _head = byte_ptr - alloc_header->padding;
}

cc::byte* cc::stack_allocator::realloc(void* ptr, cc::size_t old_size, cc::size_t new_size, cc::size_t align)
{
    if (ptr == nullptr)
        return this->alloc(new_size, align);

    CC_ASSERT(ptr == _last_alloc && "argument to stack_allocator::realloc was not the most recent allocation");
    (void)old_size;
    (void)align;
    // no need to memcpy, the memory remains the same
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);

    CC_ASSERT(byte_ptr + new_size <= _buffer_end && "stack_allocator overcommitted");

    _head = byte_ptr + new_size;
    return byte_ptr;
}

cc::byte* cc::system_allocator_t::alloc(cc::size_t size, cc::size_t align)
{
#ifdef CC_OS_WINDOWS
    return static_cast<cc::byte*>(::_aligned_malloc(size, align));
#else
    return static_cast<cc::byte*>(std::aligned_alloc(align, size));
#endif
}

void cc::system_allocator_t::free(void* ptr)
{
#ifdef CC_OS_WINDOWS
    ::_aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

cc::byte* cc::system_allocator_t::realloc(void* ptr, cc::size_t old_size, cc::size_t new_size, cc::size_t align)
{
#ifdef CC_OS_WINDOWS
    (void)old_size;
    return static_cast<cc::byte*>(::_aligned_realloc(ptr, new_size, align));
#else
    if (align == alignof(std::max_align_t))
    {
        return static_cast<cc::byte*>(std::realloc(ptr, new_size));
    }
    else
    {
        // there is no aligned_realloc equivalent on POSIX, we have to do it manually
        cc::byte* res = nullptr;

        if (new_size > 0)
        {
            res = this->alloc(new_size, align);

            if (ptr != nullptr)
            {
                std::memcpy(res, ptr, cc::min(old_size, new_size));
            }
        }

        this->free(ptr);
        return res;
    }
#endif
}

cc::system_allocator_t cc::system_allocator_instance = {};
cc::allocator* const cc::system_allocator = &cc::system_allocator_instance;

cc::byte* cc::allocator::realloc(void* ptr, cc::size_t old_size, cc::size_t new_size, cc::size_t align)
{
    cc::byte* res = nullptr;

    if (new_size > 0)
    {
        res = this->alloc(new_size, align);

        if (ptr != nullptr)
        {
            std::memcpy(res, ptr, cc::min(old_size, new_size));
        }
    }

    this->free(ptr);
    return res;
}

cc::byte* cc::allocator::alloc_request(cc::size_t min_size, cc::size_t request_size, cc::size_t& out_received_size, cc::size_t align)
{
    CC_UNUSED(request_size);
    out_received_size = min_size;
    return this->alloc(min_size, align);
}

cc::byte* cc::allocator::realloc_request(void* ptr, cc::size_t old_size, cc::size_t new_min_size, cc::size_t request_size, cc::size_t& out_received_size, cc::size_t align)
{
    CC_UNUSED(request_size);
    out_received_size = new_min_size;
    return this->realloc(ptr, old_size, new_min_size, align);
}

cc::byte* cc::scratch_allocator::alloc(cc::size_t size, cc::size_t align)
{
    CC_DTRACE_ALLOC_PRINTF("    [alloc]    call - size %zu, align %zu\n", size, align);

    size = int_ceil_to_multiple<size_t>(size, 4);

    byte* p = _head;
    scratch_alloc_header* h = reinterpret_cast<scratch_alloc_header*>(p);
    byte* data = align_up_from_header(h, align); // points to after the header, aligned
    p = data + size;                             // end of planned allocation

    // end would be OOB, wrap around to start of ring
    if (p > _buffer_end)
    {
        // write wraparound marker to the header
        CC_ASSERT(reinterpret_cast<byte*>(h) < _buffer_end && "programmer error");
        uint32_t const num_bytes_until_end = uint32_t(_buffer_end - reinterpret_cast<byte*>(h));

        CC_DTRACE_ALLOC_PRINTF("    [alloc]    %zu bytes would wrap, writing %u to wraparound header [%u]\n", size, num_bytes_until_end, get_ptr_offset(h));

        h->size = (num_bytes_until_end | gc_header_free_bit);

        p = _buffer_begin;
        h = reinterpret_cast<scratch_alloc_header*>(p);
        data = align_up_from_header(h, align); // points to after the header, aligned
        p = data + size;                       // end of planned allocation
    }

    if (p > _buffer_end || in_use(p))
    {
        // ring buffer is exhausted, use fallback
        CC_ASSERT(_backing_alloc != nullptr && "scratch_allocator out of memory and no backing allocator present");
        return _backing_alloc->alloc(size, align);
    }

    // write size to the header
    uint32 const allocsize = static_cast<uint32>(p - reinterpret_cast<byte*>(h));
    fill_in_header_value(h, data, allocsize);
    _head = p;

    if (_head == _buffer_end)
    {
        // head is at end of the buffer, and would never be reached by the free loop, wrap to begin
        _head = _buffer_begin;
    }

    CC_DTRACE_ALLOC_PRINTF("    [alloc]    head: %u, writing %u to header [%u]\n", get_ptr_offset(_head), allocsize, get_ptr_offset(h));
    CC_DTRACE_ALLOC_FFLUSH();


    return data;
}

void cc::scratch_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;


    // free from backing allocator if not in own ring buffer
    if (ptr < _buffer_begin || ptr > _buffer_end)
    {
        CC_ASSERT(_backing_alloc != nullptr && "freed out of bounds pointer with scratch allocator and no backing allocator present that could be the source of it");
        _backing_alloc->free(ptr);
        return;
    }

    // mark header of freed slot as free
    scratch_alloc_header* const h = get_header_before_pointer(ptr);

    CC_DTRACE_ALLOC_PRINTF("    [free]     freeing %u, entering loop (tail: %u, head: %u)\n", get_ptr_offset(h), get_ptr_offset(_tail), get_ptr_offset(_head));

    CC_ASSERT((h->size & gc_header_free_bit) == 0 && "scratch_allocator double free");
    h->size = h->size | gc_header_free_bit;


    // advance tail past all free slots
    while (_tail != _head)
    {
        scratch_alloc_header const* const slot_h = reinterpret_cast<scratch_alloc_header*>(_tail);

        // break once a non-free slot was reached
        if ((slot_h->size & gc_header_free_bit) == 0)
        {
            CC_DTRACE_ALLOC_PRINTF("    [free]     reached non-free header [%u] (break)\n", get_ptr_offset(slot_h));
            CC_DTRACE_ALLOC_FFLUSH();
            break;
        }

        // mask free bit out from the size, advance tail
        _tail += slot_h->size & 0x7fffffffu;


        // wrap around to start of ring
        if (_tail == _buffer_end)
        {
            CC_DTRACE_ALLOC_PRINTF("    [free]     jumped from header [%u] to %u (wrap)\n", get_ptr_offset(slot_h), get_ptr_offset(_buffer_begin));
            CC_DTRACE_ALLOC_FFLUSH();

            _tail = _buffer_begin;
        }
        else
        {
            CC_DTRACE_ALLOC_PRINTF("    [free]     jumped from header [%u] to %u (non-wrap)\n", get_ptr_offset(slot_h), get_ptr_offset(_tail));
            CC_DTRACE_ALLOC_FFLUSH();
        }
    }

    if (_tail == _head)
    {
        CC_DTRACE_ALLOC_PRINTF("    [free]     tail == head\n");
        CC_DTRACE_ALLOC_FFLUSH();
    }
}

unsigned cc::scratch_allocator::get_ptr_offset(void const* ptr) const
{
    CC_ASSERT(ptr >= _buffer_begin && "programmer error");
    return unsigned(static_cast<byte const*>(ptr) - _buffer_begin);
}
