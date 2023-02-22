#include <clean-core/allocator.hh>

#include <clean-core/macros.hh>

#if defined(CC_OS_APPLE) || defined(CC_OS_WINDOWS)
#define CC_USE_ALIGNED_MALLOC 1
#else
#define CC_USE_ALIGNED_MALLOC 0
#endif

#if !CC_USE_ALIGNED_MALLOC
#include <cstdlib>
#endif

namespace cc
{
namespace
{
// system provided allocator (malloc / free)
struct system_allocator_t final : cc::allocator
{
    std::byte* try_alloc(size_t size, size_t alignment) override
    {
        alignment = cc::max<size_t>(alignment, size >= 16 ? 16 : 8);

#if CC_USE_ALIGNED_MALLOC
        void* result = ::_aligned_malloc(size, alignment);
#else
        void* ptr = std::malloc(size + alignment + sizeof(void*) + sizeof(size_t));
        void* result = nullptr;
        if (ptr)
        {
            result = cc::align_up((uint8_t*)ptr + sizeof(void*) + sizeof(size_t), alignment);
            *((void**)((uint8_t*)result - sizeof(void*))) = ptr;
            *((size_t*)((uint8_t*)result - sizeof(void*) - sizeof(size_t))) = size;
        }
#endif

        return static_cast<std::byte*>(result);
    }

    std::byte* alloc(size_t size, size_t alignment) override
    {
        std::byte* const result = this->try_alloc(size, alignment);

        CC_RUNTIME_ASSERT((result != nullptr || size == 0) && "Out of system memory - allocation failed");

        return result;
    }

    std::byte* try_realloc(void* ptr, size_t new_size, size_t align) override
    {
        align = cc::max<size_t>(align, new_size >= 16 ? 16 : 8);

#if CC_USE_ALIGNED_MALLOC
        void* result;

        if (ptr && new_size)
        {
            result = ::_aligned_realloc(ptr, new_size, align);
        }
        else if (!ptr)
        {
            result = ::_aligned_malloc(new_size, align);
        }
        else
        {
            ::_aligned_free(ptr);
            result = nullptr;
        }

        return static_cast<std::byte*>(result);

#else // !CC_USE_ALIGNED_MALLOC
      // default realloc implementation
        return cc::allocator::realloc(ptr, new_size, align);
#endif
    }

    std::byte* realloc(void* ptr, size_t new_size, size_t align) override
    {
        std::byte* result = this->try_realloc(ptr, new_size, align);

        CC_RUNTIME_ASSERT((result != nullptr || new_size == 0) && "Out of system memory - allocation failed");

        return result;
    }

    void free(void* ptr) override
    {
#if CC_USE_ALIGNED_MALLOC
        ::_aligned_free(ptr);
#else
        if (ptr)
        {
            std::free(*((void**)((uint8_t*)ptr - sizeof(void*))));
        }
#endif
    }

    bool get_allocation_size(void const* ptr, size_t& out_size) override
    {
        if (!ptr)
        {
            return false;
        }

#if CC_USE_ALIGNED_MALLOC
        out_size = ::_aligned_msize(const_cast<void*>(ptr), 16, 0);
        return true;
#else
        out_size = *((size_t*)((uint8_t*)ptr - sizeof(void*) - sizeof(size_t)));
        return true;
#endif
    }

    bool validate_heap() override
    {
#ifdef CC_OS_WINDOWS

        int32_t const res = _heapchk();

        CC_RUNTIME_ASSERT(res != _HEAPBADBEGIN && "Heap check: Initial header information is bad or can't be found.");
        CC_RUNTIME_ASSERT(res != _HEAPBADNODE && "Heap check: Bad node has been found or heap is damaged.");
        CC_RUNTIME_ASSERT(res != _HEAPBADPTR && "Heap check: Pointer into heap isn't valid.");
        CC_RUNTIME_ASSERT(res != _HEAPEMPTY && "Heap check: Heap hasn't been initialized.");

        CC_RUNTIME_ASSERT(res == _HEAPOK && "Heap check: Unknown issue");

        return true;
#else
        return false;
#endif
    }

    char const* get_name() const override
    {
#if CC_USE_ALIGNED_MALLOC
        return "C runtime allocator (_aligned_malloc)";
#else
        return "C runtime allocator (malloc)";
#endif
    }

    constexpr system_allocator_t() = default;
};
}
}

/*
 * we must make sure that cc::system_allocator is valid during complete init and shutdown
 * this is not given if a "static cc::system_allocator" instance is used
 * in this version, the union is statically initialized and gcc and clang store the cc::system_allocator in the binary
 * the dtor does nothing, so that the vtable ptr of the allocator stays valid
 * also, as this is static data, it does not count as a memory leak
 *
 * NOTE: system_allocator_t and sys_alloc_union_t must have constexpr ctors for this to work somewhat reliably
 */
static union sys_alloc_union_t
{
    cc::system_allocator_t alloc;

    constexpr sys_alloc_union_t() : alloc() {}
    ~sys_alloc_union_t()
    { /* nothing */
    }
} sys_alloc_instance;

cc::allocator* const cc::system_allocator = &sys_alloc_instance.alloc;
