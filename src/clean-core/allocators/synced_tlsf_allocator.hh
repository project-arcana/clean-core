#pragma once

#include <clean-core/allocator.hh>

#include <clean-core/allocators/tlsf_allocator.hh>
#include <clean-core/spin_lock.hh>
#include <clean-core/lock_guard.hh>

namespace cc
{
/// Synchronized (mutexed) version of tlsf_allocator
template <class LockT = cc::spin_lock>
struct synced_tlsf_allocator final : allocator
{
    synced_tlsf_allocator() = default;
    explicit synced_tlsf_allocator(cc::span<std::byte> buffer) : _backing(buffer) {}
    ~synced_tlsf_allocator() { _backing.destroy(); }


    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = cc::lock_guard(_lock);
        return _backing.alloc(size, align);
    }

    void free(void* ptr) override
    {
        auto lg = cc::lock_guard(_lock);
        _backing.free(ptr);
    }

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = cc::lock_guard(_lock);
        return _backing.realloc(ptr, old_size, new_size, align);
    }

    void initialize(cc::span<std::byte> buffer) { _backing.initialize(buffer); }
    void destroy() { _backing.destroy(); }

private:
    LockT _lock;
    cc::tlsf_allocator _backing;
};
}