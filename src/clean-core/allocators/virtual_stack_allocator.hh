#pragma once

#include <clean-core/allocator.hh>

namespace cc
{
/// stack allocator operating in virtual memory
/// reserves pages on init, commits pages on demand
/// only frees pages if explicitly called
struct virtual_stack_allocator final : allocator
{
    virtual_stack_allocator() = default;
    explicit virtual_stack_allocator(size_t max_size_bytes, size_t chunk_size_bytes = 65536) { initialize(max_size_bytes, chunk_size_bytes); }
    ~virtual_stack_allocator() { destroy(); }

    // max_size_bytes: amount of contiguous virtual memory being reserved
    // chunk_size_bytes: increment of physical memory being committed whenever more is required
    // note there is a lower limit on virtual allocation granularity (Win32: 64K = 16 pages)
    void initialize(size_t max_size_bytes, size_t chunk_size_bytes = 65536);

    void destroy();

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    /// NOTE: ptr must be the most recent allocation received
    void free(void* ptr) override;

    /// NOTE: ptr must be the most recent allocation received
    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    // free all current allocations
    // does not decommit any memory!
    size_t reset();

    // decommit the physical memory of all pages not currently required
    // returns amount of bytes decommitted
    size_t decommit_idle_memory();

    // amount of bytes in the virtual address range
    size_t get_virtual_size_bytes() const { return _virtual_end - _virtual_begin; }

    // amount of bytes in the physically committed memory
    size_t get_physical_size_bytes() const { return _physical_end - _virtual_begin; }

    // amount of bytes in the physically committed and allocated memory
    size_t get_allocated_size_bytes() const { return _physical_current - _virtual_begin; }

    // returns whether the given ptr is the latest allocation, meaning it can be freed or reallocated
    bool is_latest_allocation(void* ptr) const;

private:
    std::byte* _virtual_begin = nullptr;
    std::byte* _virtual_end = nullptr;
    std::byte* _physical_current = nullptr;
    std::byte* _physical_end = nullptr;
    int32_t _last_alloc_id = 0;
    size_t _chunk_size_bytes = 0;
};
}