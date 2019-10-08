#pragma once

#include <clean-core/assert.hh>
#include <clean-core/fwd.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
// a view on an utf-8 string
// is NON-OWNING
// is a view and CANNOT modify the content of the string
// this class is cheap to copy, passing it by reference has no benefits
struct string_view
{
    constexpr string_view() = default;
    template <size_t N> // null-terminated string literal
    constexpr string_view(char const (&data)[N]) : _data(data), _size(N - 1)
    {
    }
    explicit string_view(char const* data);
    // TODO: more ctors

    // container
public:
    constexpr char const* begin() const { return _data; }
    constexpr char const* end() const { return _data + _size; }
    constexpr bool empty() const { return _size == 0; }
    constexpr char const* data() const { return _data; }
    constexpr size_t size() const { return _size; }

    constexpr char const& operator[](size_t idx) const
    {
        CC_CONTRACT(idx < _size);
        return _data[idx];
    }

private:
    char const* _data = nullptr;
    size_t _size = 0;
};
}
