#include <clean-core/allocator.hh>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

// enable debug trace prints in some allocators
#define CC_ENABLE_DTRACE_ALLOC 0

#if CC_ENABLE_DTRACE_ALLOC
#define CC_DTRACE_ALLOC_PRINTF printf
#define CC_DTRACE_ALLOC_FFLUSH() fflush(stdout)
#else
#define CC_DTRACE_ALLOC_PRINTF(...) (void)0
#define CC_DTRACE_ALLOC_FFLUSH() (void)0
#endif


namespace
{
struct scratch_alloc_header
{
    uint32_t size;
};

constexpr uint32_t const gc_header_pad_value = 0xffffffffu;
constexpr uint32_t const gc_header_free_bit = 0x80000000u;

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


/*
 * cc::scratch_allocator is based on the bitsquid foundation ScratchAllocator
 * https://github.com/niklas-ourmachinery/bitsquid-foundation/blob/master/memory.cpp#L142
 * the original implementation contains bugs which are fixed here
 *
 * original license (MIT):
 *
 * Copyright (C) 2012 Bitsquid AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
