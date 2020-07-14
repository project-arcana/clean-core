#include <clean-core/allocator.hh>

#include <cstdlib>
#include <cstring>

#include <clean-core/assert.hh>

namespace
{
struct stack_alloc_header
{
    size_t padding;
};

template <class T>
[[nodiscard]] T align_up_masked(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <class T>
[[nodiscard]] T align_up(T value, size_t alignment)
{
    return align_up_masked(value, alignment - 1);
}

inline std::byte* align_up_with_header(std::byte* head, size_t align, size_t header_size)
{
    std::byte* padded_res = align_up(head, align);
    size_t const padding = size_t(padded_res - head);

    if (padding < header_size)
    {
        // header does not fit - align up
        header_size -= padding;

        if (header_size % align > 0)
        {
            padded_res += align * (1 + (header_size / align));
        }
        else
        {
            padded_res += align * (header_size / align);
        }
    }

    return padded_res;
}
}

cc::byte* cc::linear_allocator::alloc(cc::size_t size, cc::size_t align)
{
    CC_ASSERT(_buffer != nullptr && "linear_allocator uninitialized");

    auto* const padded_res = align_up(_head, align);

    if (CC_UNLIKELY(padded_res + size > _buffer_end))
    {
        CC_ASSERT(false && "linear_allocator overcommited");
        return nullptr;
    }

    _head = padded_res + size;
    return padded_res;
}

cc::byte* cc::stack_allocator::alloc(cc::size_t size, cc::size_t align)
{
    CC_ASSERT(_buffer != nullptr && "stack_allocator uninitialized");

    auto* const padded_res = align_up_with_header(_head, align, sizeof(stack_alloc_header));

    if (CC_UNLIKELY(padded_res + size > _buffer_end))
    {
        CC_ASSERT(false && "stack_allocator overcommited");
        return nullptr;
    }

    stack_alloc_header const header{size_t(padded_res - _head)};
    std::memcpy(padded_res - sizeof(header), &header, sizeof(header));

    _head = padded_res + size;
    _last_alloc = padded_res;
    return padded_res;
}

void cc::stack_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;

    CC_ASSERT(ptr == _last_alloc && "argument to stack_allocator::free was not the most recent allocation");

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));
    _head = byte_ptr - alloc_header->padding;
}

cc::byte* cc::system_allocator_t::alloc(cc::size_t size, cc::size_t align)
{
#ifdef CC_OS_WINDOWS
    return static_cast<cc::byte*>(::_aligned_malloc(size, align));
#else
    return static_cast<cc::byte*>(std::aligned_alloc(align, size));
#endif
}

void cc::system_allocator_t::free(void* ptr)
{
#ifdef CC_OS_WINDOWS
    ::_aligned_free(ptr);
#else
    std::free(ptr);
#endif
}
