#pragma once

#include <clean-core/allocator.hh>

#include <clean-core/allocators/tlsf_allocator.hh>
#include <clean-core/lock_guard.hh>
#include <clean-core/spin_lock.hh>

namespace cc
{
/// Synchronized (mutexed) version of tlsf_allocator
template <class LockT = cc::spin_lock>
struct synced_tlsf_allocator final : allocator
{
    synced_tlsf_allocator() = default;
    explicit synced_tlsf_allocator(cc::span<std::byte> buffer) : _backing(buffer) {}
    ~synced_tlsf_allocator() { destroy(); }


    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = cc::lock_guard(_lock);
        return _backing.alloc(size, align);
    }

    // alloc and report internal size (without double locking)
    std::byte* alloc(size_t size, size_t align, size_t* out_internal_size)
    {
        auto lg = cc::lock_guard(_lock);
        std::byte* const res = _backing.alloc(size, align);
        _backing.get_allocation_size(res, *out_internal_size);
        return res;
    }

    void free(void* ptr) override
    {
        auto lg = cc::lock_guard(_lock);
        _backing.free(ptr);
    }

    // free and report previous internal size (without double locking)
    void free(void* ptr, size_t* out_internal_size)
    {
        auto lg = cc::lock_guard(_lock);
        _backing.get_allocation_size(ptr, *out_internal_size);
        _backing.free(ptr);
    }

    std::byte* realloc(void* ptr, size_t new_size, size_t align = alignof(std::max_align_t)) override
    {
        auto lg = cc::lock_guard(_lock);
        return _backing.realloc(ptr, new_size, align);
    }

    bool get_allocation_size(void const* ptr, size_t& out_size) override
    {
        auto lg = cc::lock_guard(_lock);
        return _backing.get_allocation_size(ptr, out_size);
    }

    bool validate_heap() override
    {
        auto lg = cc::lock_guard(_lock);
        return _backing.validate_heap();
    }

    char const* get_name() const override { return "Synced TLSF Allocator"; }

    void initialize(cc::span<std::byte> buffer) { _backing.initialize(buffer); }
    void destroy()
    {
        auto lg = cc::lock_guard(_lock);
        _backing.destroy();
    }

private:
    LockT _lock;
    cc::tlsf_allocator _backing;
};
} // namespace cc