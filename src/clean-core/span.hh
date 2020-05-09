#pragma once

#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/fwd.hh>
#include <clean-core/is_contiguous_range.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
// a non-owning view of a contiguous array of Ts
// can be read and write (span<const T> vs span<T>)
// is trivially copyable (and cheap)
// NOTE: is range-checked via CC_CONTRACT
template <class T>
struct span
{
    // ctors
public:
    constexpr span() = default;
    constexpr span(T* data, size_t size) : _data(data), _size(size) {}
    constexpr span(T* d_begin, T* d_end) : _data(d_begin), _size(d_end - d_begin) {}
    template <size_t N>
    constexpr span(T (&data)[N]) : _data(data), _size(N)
    {
    }
    template <class Container, std::enable_if_t<is_contiguous_range<Container, T>, int> = 0>
    constexpr span(Container& c) : _data(c.data()), _size(c.size())
    {
    }

    explicit constexpr span(T& val) : _data(&val), _size(1) {}

    constexpr operator span<T const>() const noexcept { return {_data, _size}; }

    // container
public:
    constexpr T* begin() const { return _data; }
    constexpr T* end() const { return _data + _size; }

    constexpr T* data() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr size_t size_bytes() const { return _size * sizeof(T); }
    constexpr bool empty() const { return _size == 0; }

    constexpr T& operator[](size_t i) const
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }

    constexpr T& front() const
    {
        CC_CONTRACT(_size > 0);
        return _data[0];
    }
    constexpr T& back() const
    {
        CC_CONTRACT(_size > 0);
        return _data[_size - 1];
    }

    // subviews
public:
    constexpr span first(size_t n) const
    {
        CC_CONTRACT(n <= _size);
        return {_data, n};
    }
    constexpr span last(size_t n) const
    {
        CC_CONTRACT(n <= _size);
        return {_data + (_size - n), n};
    }
    constexpr span subspan(size_t offset, size_t count) const
    {
        CC_CONTRACT(offset <= _size && offset + count <= _size);
        return {_data + offset, count};
    }
    constexpr span subspan(size_t offset) const
    {
        CC_CONTRACT(offset <= _size);
        return {_data + offset, _size - offset};
    }

    constexpr span<byte const> as_bytes() const { return {reinterpret_cast<byte const*>(_data), _size * sizeof(T)}; }
    template <class U = T, cc::enable_if<!std::is_const_v<U>> = true>
    constexpr span<byte> as_writable_bytes() const
    {
        return {reinterpret_cast<byte*>(_data), _size * sizeof(T)};
    }

private:
    T* _data = nullptr;
    size_t _size = 0;
};

// deduction guide for containers
template <class Container, cc::enable_if<is_contiguous_range<Container, void>> = true>
span(Container& c)->span<std::remove_reference_t<decltype(*c.data())>>;
span(string_view const&)->span<char const>;

/// converts a triv. copyable value, or a container with triv. copyable elements to a cc::span<std::byte>
template <class T>
span<byte> as_byte_span(T& value)
{
    if constexpr (is_contiguous_range<T, void>)
    {
        // container of some type
        using ElementT = std::remove_reference_t<decltype(value.data()[0])>;
        static_assert(std::is_trivially_copyable_v<ElementT>, "cannot convert range of non-trivially copyable elements to byte span");
        return span<byte>{reinterpret_cast<byte*>(value.data()), sizeof(ElementT) * value.size()};
    }
    else
    {
        // single POD type
        static_assert(std::is_trivially_copyable_v<T>, "cannot convert non-trivially copyable element to byte span");
        return span<byte>{reinterpret_cast<byte*>(&value), sizeof(T)};
    }
}

/// converts a constant triv. copyable value, or a constant container with triv. copyable elements to a cc::span<std::byte const>
template <class T>
span<byte const> as_byte_span(T const& value)
{
    if constexpr (is_contiguous_range<T const, void const>)
    {
        // container of some type
        using ElementT = std::remove_reference_t<decltype(value.data()[0])>;
        static_assert(std::is_trivially_copyable_v<ElementT>, "cannot convert range of non-trivially copyable elements to byte span");
        return span<byte const>{reinterpret_cast<byte const*>(value.data()), sizeof(ElementT) * value.size()};
    }
    else
    {
        // single POD type
        static_assert(std::is_trivially_copyable_v<T>, "cannot convert non-trivially copyable element to byte span");
        return span<byte const>{reinterpret_cast<byte const*>(&value), sizeof(T)};
    }
}
}
