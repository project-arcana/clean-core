#pragma once

#include <clean-core/allocator.hh>

namespace cc
{
//
// underlying logic of cc::system_allocator_t as free functions
// required for some pre-main scenarios (e.g. non-alloc vector)

std::byte* system_malloc(size_t size, size_t alignment);

std::byte* system_realloc(void* ptr, size_t new_size, size_t align);

size_t system_msize(void const* ptr);

void system_free(void* ptr);

// system provided allocator (malloc / free)
struct system_allocator_t final : cc::allocator
{
    std::byte* try_alloc(size_t size, size_t alignment) override { return cc::system_malloc(size, alignment); }

    std::byte* alloc(size_t size, size_t alignment) override
    {
        std::byte* const result = this->try_alloc(size, alignment);
        CC_RUNTIME_ASSERT((result != nullptr || size == 0) && "Out of system memory - allocation failed");
        return result;
    }

    std::byte* try_realloc(void* ptr, size_t new_size, size_t align) override { return cc::system_realloc(ptr, new_size, align); }

    std::byte* realloc(void* ptr, size_t new_size, size_t align) override
    {
        std::byte* result = this->try_realloc(ptr, new_size, align);
        CC_RUNTIME_ASSERT((result != nullptr || new_size == 0) && "Out of system memory - allocation failed");
        return result;
    }

    void free(void* ptr) override { cc::system_free(ptr); }

    bool get_allocation_size(void const* ptr, size_t& out_size) override
    {
        out_size = cc::system_msize(ptr);
        return true;
    }

    bool validate_heap() override;

    char const* get_name() const override;

    constexpr system_allocator_t() = default;
};
}
