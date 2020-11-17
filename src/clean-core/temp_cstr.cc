#include "temp_cstr.hh"

#include <cstring>

cc::temp_cstr::temp_cstr(cc::string_view sv, cc::allocator* allocator)
{
    CC_ASSERT(allocator && "allocator is currently required for string views");
    init_from_allocator(sv, allocator);
}

cc::temp_cstr::temp_cstr(cc::string_view sv, span<std::byte> buffer, cc::allocator* fallback_allocator)
{
    if (sv.size() < buffer.size()) // local buffer enough
    {
        std::memcpy(buffer.data(), sv.data(), sv.size());
        buffer[sv.size()] = std::byte('\0');
        _data = reinterpret_cast<char const*>(buffer.data());
    }
    else
    {
        CC_ASSERT(fallback_allocator && "no fallback allocator provided");
        init_from_allocator(sv, fallback_allocator);
    }
}

cc::temp_cstr::temp_cstr(cc::string_view sv, span<char> buffer, cc::allocator* fallback_allocator)
  : temp_cstr(sv, as_byte_span(buffer), fallback_allocator)
{
}

void cc::temp_cstr::init_from_allocator(cc::string_view sv, cc::allocator* allocator)
{
    CC_ASSERT(allocator);
    _alloc = allocator;
    auto data = allocator->new_array<char>(sv.size() + 1);
    std::memcpy(data, sv.data(), sv.size());
    data[sv.size()] = '\0';
    _data = data;
}
