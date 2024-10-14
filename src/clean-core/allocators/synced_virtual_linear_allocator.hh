#pragma once

#include <clean-core/allocator.hh>
#include <clean-core/spin_lock.hh>
#include <clean-core/lock_guard.hh>
#include <clean-core/allocators/virtual_linear_allocator.hh>

namespace cc
{
/// Synchronized (mutexed) version of virtual_linear_allocator
template <class MutexT = cc::spin_lock>
struct synced_virtual_linear_allocator final : allocator
{
    synced_virtual_linear_allocator() = default;
    explicit synced_virtual_linear_allocator(size_t max_size_bytes, size_t chunk_size_bytes = 65536)
    {
        _backing.initialize(max_size_bytes, chunk_size_bytes);
    }

    void initialize(size_t max_size_bytes, size_t chunk_size_bytes = 65536) { _backing.initialize(max_size_bytes, chunk_size_bytes); }

    void destroy()
    {
        auto lg = cc::lock_guard(_mutex);
        _backing.destroy();
    }

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = cc::lock_guard(_mutex);
        return _backing.alloc(size, align);
    }

    void free(void*) override {} // nothing

    std::byte* realloc(void* ptr, size_t new_size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = cc::lock_guard(_mutex);
        return _backing.realloc(ptr, new_size, align);
    }

    char const* get_name() const override { return "Synced Virtual Linear Allocator"; }

    size_t reset()
    {
        auto lg = cc::lock_guard(_mutex);
        return _backing.reset();
    }

    // decommit the physical memory of all pages not currently required
    // returns amount of bytes decommitted
    size_t decommit_idle_memory()
    {
        auto lg = cc::lock_guard(_mutex);
        return _backing.decommit_idle_memory();
    }

    struct usage_stats
    {
        size_t num_bytes_virtual = 0;
        size_t num_bytes_physical = 0;
        size_t num_bytes_allocated = 0;
    };

    usage_stats get_usage_stats()
    {
        auto lg = cc::lock_guard(_mutex);
        usage_stats res = {};
        res.num_bytes_virtual = _backing.get_virtual_size_bytes();
        res.num_bytes_physical = _backing.get_physical_size_bytes();
        res.num_bytes_allocated = _backing.get_allocated_size_bytes();
        return res;
    }


private:
    MutexT _mutex;
    cc::virtual_linear_allocator _backing;
};
} // namespace cc