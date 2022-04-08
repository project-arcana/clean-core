#include <clean-core/allocator.hh>


namespace
{
/// system provided allocator (malloc / free)
struct system_allocator_t final : cc::allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
#ifdef CC_OS_WINDOWS
        return static_cast<std::byte*>(::_aligned_malloc(size, align));
#else
        return static_cast<std::byte*>(std::aligned_alloc(align, size));
#endif
    }

    void free(void* ptr) override
    {
#ifdef CC_OS_WINDOWS
        ::_aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override
    {
#ifdef CC_OS_WINDOWS
        (void)old_size;
        return static_cast<std::byte*>(::_aligned_realloc(ptr, new_size, align));
#else
        if (align == alignof(std::max_align_t))
        {
            return static_cast<std::byte*>(std::realloc(ptr, new_size));
        }
        else
        {
            // there is no aligned_realloc equivalent on POSIX, we have to do it manually
            std::byte* res = nullptr;

            if (new_size > 0)
            {
                res = this->alloc(new_size, align);

                if (ptr != nullptr)
                {
                    std::memcpy(res, ptr, cc::min(old_size, new_size));
                }
            }

            this->free(ptr);
            return res;
        }
#endif
    }

    constexpr system_allocator_t() = default;
};

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
    system_allocator_t alloc;

    constexpr sys_alloc_union_t() : alloc() {}
    ~sys_alloc_union_t()
    { /* nothing */
    }
} sys_alloc_instance;

cc::allocator* const cc::system_allocator = &sys_alloc_instance.alloc;
