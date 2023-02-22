#include "system_allocator.hh"

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

std::byte* cc::system_malloc(size_t size, size_t alignment)
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

std::byte* cc::system_realloc(void* ptr, size_t new_size, size_t align)
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
    std::byte* res = nullptr;
    if (new_size > 0)
    {
        res = cc::system_malloc(new_size, align);

        if (ptr != nullptr)
        {
            size_t const old_size = cc::system_msize(ptr);
            std::memcpy(res, ptr, cc::min(old_size, new_size));
        }
    }

    cc::system_free(ptr);
    return res;
#endif
}

size_t cc::system_msize(void const* ptr)
{
    if (!ptr)
    {
        return 0;
    }

#if CC_USE_ALIGNED_MALLOC
    return ::_aligned_msize(const_cast<void*>(ptr), 16, 0);
#else
    return *((size_t*)((uint8_t*)ptr - sizeof(void*) - sizeof(size_t)));
#endif
}

void cc::system_free(void* ptr)
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

bool cc::system_allocator_t::validate_heap()
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

char const* cc::system_allocator_t::get_name() const
{
#if CC_USE_ALIGNED_MALLOC
    return "C runtime allocator (_aligned_malloc)";
#else
    return "C runtime allocator (malloc)";
#endif
}
