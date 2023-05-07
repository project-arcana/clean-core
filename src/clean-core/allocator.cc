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
    char* const res = reinterpret_cast<char*>(this->alloc(source_length + 1, alignof(char)));

    // check because empty source is a legitimate case (allocates a single 0 char)
    if (source_length > 0)
    {
        std::memcpy(res, source.data(), source_length);
    }
    res[source_length] = '\0';
    return res;
}

std::byte* cc::allocator::realloc(void* ptr, size_t new_size, size_t align)
{
    std::byte* res = nullptr;

    if (new_size > 0)
    {
        res = this->alloc(new_size, align);

        if (ptr != nullptr)
        {
            size_t old_size = 0;
            bool got_old_size = this->get_allocation_size(ptr, old_size);
            CC_RUNTIME_ASSERT(got_old_size && "Allocator with default realloc failed to provide old allocation size");

            std::memcpy(res, ptr, cc::min(old_size, new_size));
        }
    }

    this->free(ptr);
    return res;
}

std::byte* cc::allocator::try_alloc(size_t size, size_t alignment) { return this->alloc(size, alignment); }

std::byte* cc::allocator::try_realloc(void* ptr, size_t new_size, size_t alignment) { return this->realloc(ptr, new_size, alignment); }
