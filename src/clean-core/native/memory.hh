#pragma once

#include <cstddef>

namespace cc
{
// reserves a range of pages in virtual memory
std::byte* reserve_virtual_memory(size_t size_bytes);

// frees a virtual memory range, the result of reserve_virtual_memory
// also decommits all physical regions inside
void free_virtual_memory(std::byte* ptr, size_t size_bytes);

// commits a region of pages inside a reserved, virtual memory range
void commit_physical_memory(std::byte* ptr, size_t size_bytes);

// decommits a region of pages inside a reserved, virtual memory range
void decommit_physical_memory(std::byte* ptr, size_t size_bytes);
}