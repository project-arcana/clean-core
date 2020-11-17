#pragma once

#include <type_traits>

#include <clean-core/allocator.hh>
#include <clean-core/fwd.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

namespace cc
{
namespace detail
{
template <class T, class = void>
struct has_c_str : std::false_type
{
};
template <class T>
struct has_c_str<T, std::void_t<decltype(static_cast<char const*>(std::declval<T>().c_str()))>> : std::true_type
{
};
}

/**
 * A utility class for passing c strings to external APIs
 *
 * Usage:
 *
 *   void external_fun(char const*);
 *
 *   external_fun(cc::temp_cstr(sv));
 *   // where sv is a string_view or string-like object
 *
 * NOTE: temp_cstr behaves like a conditional view-type
 *       it must not outlive its parameter
 *
 * TODO: if string_view becomes cstring-aware, we can save a copy
 */
struct temp_cstr
{
    operator char const*() const { return _data; }

    /// currently, always allocates a temporary array for string_views
    /// (will change if string_views get c-string-aware)
    explicit temp_cstr(string_view sv, cc::allocator* allocator = cc::system_allocator);
    /// same as temp_cstr(string_view sv, cc::allocator*) but tries to use the provided buffer first
    explicit temp_cstr(string_view sv, span<std::byte> buffer, cc::allocator* fallback_allocator = cc::system_allocator);
    /// same as temp_cstr(string_view sv, cc::allocator*) but tries to use the provided buffer first
    explicit temp_cstr(string_view sv, span<char> buffer, cc::allocator* fallback_allocator = cc::system_allocator);
    /// non-allocating version: just passes through the given c string
    explicit temp_cstr(char const* s) : _data(s) {}
    /// non-allocating version: objects that follow the "c_str()"-protocol
    template <class String, enable_if<cc::detail::has_c_str<String>::value> = true>
    explicit temp_cstr(String&& str) : _data(str.c_str())
    {
    }

    temp_cstr(temp_cstr const&) = delete;
    temp_cstr(temp_cstr&&) = delete;
    temp_cstr& operator=(temp_cstr const&) = delete;
    temp_cstr& operator=(temp_cstr&&) = delete;

    ~temp_cstr()
    {
        // NOTE: the const_cast is fine as this case is only hit if we actually allocated the data from _alloc
        if (_data && _alloc)
            _alloc->delete_array(reinterpret_cast<std::byte*>(const_cast<char*>(_data)));
    }

private:
    char const* _data = nullptr;
    cc::allocator* _alloc = nullptr;

    void init_from_allocator(string_view sv, cc::allocator* allocator);
};
}
