#pragma once

#include <clean-core/allocator.hh>

namespace cc
{
/// linear allocator operating in virtual memory
/// reserves pages on init, commits pages on demand
/// only frees pages if explicitly called
struct virtual_linear_allocator final : allocator
{
    virtual_linear_allocator() = default;
    explicit virtual_linear_allocator(size_t max_size_bytes, size_t chunk_size_bytes = 65536) { initialize(max_size_bytes, chunk_size_bytes); }
    ~virtual_linear_allocator() { destroy(); }

    // max_size_bytes: amount of contiguous virtual memory being reserved
    // chunk_size_bytes: increment of physical memory being committed whenever more is required
    // note there is a lower limit on virtual allocation granularity (Win32: 64K = 16 pages)
    void initialize(size_t max_size_bytes, size_t chunk_size_bytes = 65536);

    void destroy();

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override { (void)ptr; }

    bool get_allocation_size(void const* ptr, size_t& out_size) override;

    char const* get_name() const override { return "Virtual Linear Allocator"; }

    std::byte* realloc(void* ptr, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    // free all current allocations
    // does not decommit any memory!
    size_t reset()
    {
        size_t const num_bytes_allocated = _physical_current - _virtual_begin;
        _physical_current = _virtual_begin;
        _last_allocation = nullptr;
        return num_bytes_allocated;
    }

    // decommit the physical memory of all pages not currently required
    // returns amount of bytes decommitted
    size_t decommit_idle_memory();

    // amount of bytes in the virtual address range
    size_t get_virtual_size_bytes() const { return _virtual_end - _virtual_begin; }

    // amount of bytes in the physically committed memory
    size_t get_physical_size_bytes() const { return _physical_end - _virtual_begin; }

    // amount of bytes in the physically committed and allocated memory
    size_t get_allocated_size_bytes() const { return _physical_current - _virtual_begin; }

private:
    std::byte* _virtual_begin = nullptr;
    std::byte* _virtual_end = nullptr;
    std::byte* _physical_current = nullptr;
    std::byte* _physical_end = nullptr;
    std::byte* _last_allocation = nullptr;
    size_t _chunk_size_bytes = 0;
};
}