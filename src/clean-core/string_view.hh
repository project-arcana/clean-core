#pragma once

#include <clean-core/assert.hh>
#include <clean-core/detail/is_contiguous_container.hh>
#include <clean-core/fwd.hh>
#include <clean-core/typedefs.hh>

#include <type_traits>

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
    explicit string_view(char const* data, size_t size) : _data(data), _size(size) {}

    template <class ContainerT, class = std::enable_if_t<is_contiguous_container<ContainerT, char>>>
    explicit string_view(ContainerT const& c) : _data(c.data()), _size(c.size())
    {
    }

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

public:
    constexpr bool operator==(string_view rhs) const
    {
        if (_size != rhs._size)
            return false;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs._data[i])
                return false;
        return true;
    }
    constexpr bool operator!=(string_view rhs) const
    {
        if (_size != rhs._size)
            return true;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs._data[i])
                return true;
        return false;
    }

    template <size_t N>
    constexpr bool operator==(char const (&rhs)[N]) const
    {
        if (N - 1 != _size)
            return false;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs[i])
                return false;
        return true;
    }
    template <size_t N>
    constexpr bool operator!=(char const (&rhs)[N]) const
    {
        if (N - 1 != _size)
            return true;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs[i])
                return true;
        return false;
    }

private:
    char const* _data = nullptr;
    size_t _size = 0;
};
}
