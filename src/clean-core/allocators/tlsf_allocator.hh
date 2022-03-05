#pragma once

#include <clean-core/allocator.hh>

namespace cc
{
/// Two Level Segregated Fit allocator
/// O(1) cost for alloc, free, realloc
/// extremely low memory overhead, 4 byte per allocation
struct tlsf_allocator final : allocator
{
    tlsf_allocator() = default;
    explicit tlsf_allocator(cc::span<std::byte> buffer) { initialize(buffer); }
    ~tlsf_allocator() { destroy(); }

    void initialize(cc::span<std::byte> buffer);
    void destroy();

    // provide an additional memory pool to the TLSF (can be done multiple times)
    void add_pool(cc::span<std::byte> buffer);

    // returns false if internal consistency checks fail
    bool check_consistency();

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override;

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    tlsf_allocator(tlsf_allocator&& rhs) noexcept : _tlsf(rhs._tlsf) { rhs._tlsf = nullptr; }
    tlsf_allocator& operator==(tlsf_allocator&& rhs) noexcept
    {
        destroy();
        _tlsf = rhs._tlsf;
        rhs._tlsf = nullptr;
        return *this;
    }

private:
    void* _tlsf = nullptr;
};
}