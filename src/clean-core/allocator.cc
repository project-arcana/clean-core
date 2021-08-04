#include <clean-core/allocator.hh>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/string_view.hh>
#include <clean-core/utility.hh>

namespace
{
struct stack_alloc_header
{
    uint32_t padding;
    int32_t alloc_id;
};

// [... pad ...] [header] [data]
std::byte* align_up_with_header(std::byte* head, size_t align, size_t header_size)
{
    std::byte* padded_res = cc::align_up(head, align);
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


std::byte* cc::stack_allocator::alloc(size_t size, size_t align)
{
    CC_ASSERT(_buffer_begin != nullptr && "stack_allocator uninitialized");

    auto* const padded_res = align_up_with_header(_head, align, sizeof(stack_alloc_header));

    CC_ASSERT(padded_res + size <= _buffer_end && "stack_allocator overcommitted");

    ++_last_alloc_id;
    stack_alloc_header header = {};
    header.padding = uint32_t(padded_res - _head);
    header.alloc_id = _last_alloc_id;

    std::memcpy(padded_res - sizeof(header), &header, sizeof(header));

    _head = padded_res + size;
    return padded_res;
}

void cc::stack_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    CC_ASSERT(alloc_header->alloc_id == _last_alloc_id && "freed ptr was not the most recent allocation");
    --_last_alloc_id;

    _head = byte_ptr - alloc_header->padding;
}

std::byte* cc::stack_allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
{
    if (new_size == 0)
    {
        // free case
        free(ptr);
        return nullptr;
    }
    else if (ptr == nullptr)
    {
        // malloc case
        return this->alloc(new_size, align);
    }

    (void)old_size;
    (void)align;
    // no need to memcpy, the memory remains the same
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    stack_alloc_header const* const alloc_header = (stack_alloc_header*)(byte_ptr - sizeof(stack_alloc_header));

    CC_ASSERT(alloc_header->alloc_id == _last_alloc_id && "realloc ptr was not the most recent allocation");
    CC_ASSERT(byte_ptr + new_size <= _buffer_end && "stack_allocator overcommitted");
    CC_ASSERT(old_size == _head - byte_ptr && "incorrect old size");

    _head = byte_ptr + new_size;
    return byte_ptr;
}

std::byte* cc::system_allocator_t::alloc(size_t size, size_t align)
{
#ifdef CC_OS_WINDOWS
    return static_cast<std::byte*>(::_aligned_malloc(size, align));
#else
    return static_cast<std::byte*>(std::aligned_alloc(align, size));
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

std::byte* cc::system_allocator_t::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
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

/*
 * we must make sure that cc::system_allocator is valid during complete init and shutdown
 * this is not given if a "static cc::system_allocator" instance is used
 * in this version, the union is statically initialized and gcc and clang store the cc::system_allocator in the binary
 * the dtor does nothing, so that the vtable ptr of the allocator stays valid
 * also, as this is static data, it does not count as a memory leak
 */
static union sys_alloc_union_t
{
    cc::system_allocator_t alloc;

    sys_alloc_union_t() : alloc() {}
    ~sys_alloc_union_t()
    { /* nothing */
    }
} sys_alloc_instance;

cc::allocator* const cc::system_allocator = &sys_alloc_instance.alloc;

char* cc::allocator::alloc_string_copy(cc::string_view source)
{
    size_t const source_length = source.length();
    char* const res = reinterpret_cast<char*>(alloc(source_length + 1));

    // check because empty source is a legitimate case (allocates a single 0 char)
    if (source_length > 0)
    {
        std::memcpy(res, source.data(), source_length);
    }
    res[source_length] = '\0';
    return res;
}

std::byte* cc::allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
{
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

std::byte* cc::allocator::alloc_request(size_t min_size, size_t request_size, size_t& out_received_size, size_t align)
{
    CC_UNUSED(request_size);
    out_received_size = min_size;
    return this->alloc(min_size, align);
}

std::byte* cc::allocator::realloc_request(void* ptr, size_t old_size, size_t new_min_size, size_t request_size, size_t& out_received_size, size_t align)
{
    CC_UNUSED(request_size);
    out_received_size = new_min_size;
    return this->realloc(ptr, old_size, new_min_size, align);
}
