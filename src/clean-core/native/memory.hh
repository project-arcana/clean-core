#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/utility.hh>

namespace cc
{
// reserves a range of pages in virtual memory
std::byte* reserve_virtual_memory(size_t size_bytes);

// frees a virtual memory range, the result of reserve_virtual_memory
// also decommits all physical regions inside
void free_virtual_memory(std::byte* ptr, size_t size_bytes);

// commits a region of pages inside a reserved, virtual memory range
void commit_physical_memory(std::byte* ptr, size_t size_bytes);

// touches (reads) the first byte of every page in this range to ensure pages are available
void prefault_memory(std::byte* ptr, size_t size_bytes);

// decommits a region of pages inside a reserved, virtual memory range
void decommit_physical_memory(std::byte* ptr, size_t size_bytes);

// commits new region of pages in multiples of a given chunk size
// checks if virtual_end is exceeded, returns the new physical end ptr
std::byte* grow_physical_memory(std::byte* physical_current, std::byte* physical_end, std::byte* virtual_end, size_t chunk_size, size_t grow_num_bytes);

// header for an allocation in a stack-like allocator
struct stack_alloc_header
{
    uint32_t padding;
    int32_t alloc_id;
};

// [... pad ...] [header] [data]
inline std::byte* align_up_with_header(std::byte* head, size_t align, size_t header_size)
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
}
