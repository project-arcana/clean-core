#include <clean-core/allocator.hh>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/string_view.hh>
#include <clean-core/utility.hh>

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
