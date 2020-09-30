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

    // prevent the templated constructor to hijack copy
    constexpr span(span const& rhs) noexcept = default;

    CC_FORCE_INLINE constexpr span(T* data, size_t size) : _data(data), _size(size) {}

    CC_FORCE_INLINE constexpr span(T* d_begin, T* d_end) : _data(d_begin), _size(d_end - d_begin) {}

    template <size_t N>
    CC_FORCE_INLINE constexpr span(T (&data)[N]) : _data(data), _size(N)
    {
    }

    /// generic span constructor from contiguous_range
    /// CAUTION: container MUST outlive the span!
    template <class Container, cc::enable_if<is_contiguous_range<Container, T>> = true>
    CC_FORCE_INLINE constexpr span(Container&& c) : _data(c.data()), _size(c.size())
    {
    }

    CC_FORCE_INLINE explicit constexpr span(T& val) : _data(&val), _size(1) {}

    /// CAUTION: value MUST outlive the span!
    /// NOTE: this ctor is for spans constructed inside an expression
    CC_FORCE_INLINE explicit constexpr span(T&& val) : _data(&val), _size(1) {}

    CC_FORCE_INLINE constexpr operator span<T const>() const noexcept { return {_data, _size}; }

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
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
span(Container& c)->span<std::remove_reference_t<decltype(*c.data())>>;
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
span(Container&& c)->span<std::remove_reference_t<decltype(*c.data())>>;

/// converts a triv. copyable value, or a container with triv. copyable elements to a cc::span<std::byte>
template <class T>
auto as_byte_span(T&& value)
{
    if constexpr (is_contiguous_range<T, void>)
    {
        // container of some type
        using ElementT = std::remove_reference_t<decltype(value.data()[0])>;
        static_assert(std::is_trivially_copyable_v<ElementT>, "cannot convert range of non-trivially copyable elements to byte span");
        return span<byte>{reinterpret_cast<byte*>(value.data()), sizeof(ElementT) * value.size()};
    }
    else if constexpr (is_contiguous_range<T, void const>)
    {
        // container of some type
        using ElementT = std::remove_reference_t<decltype(value.data()[0])>;
        static_assert(std::is_trivially_copyable_v<ElementT>, "cannot convert range of non-trivially copyable elements to byte span");
        return span<byte const>{reinterpret_cast<byte const*>(value.data()), sizeof(ElementT) * value.size()};
    }
    else if constexpr (std::is_const_v<std::remove_reference_t<T>>)
    {
        // single POD type
        static_assert(std::is_trivially_copyable_v<std::remove_reference_t<T>>, "cannot convert non-trivially copyable element to byte span");
        return span<byte const>{reinterpret_cast<byte const*>(&value), sizeof(T)};
    }
    else
    {
        // single POD type
        static_assert(std::is_trivially_copyable_v<std::remove_reference_t<T>>, "cannot convert non-trivially copyable element to byte span");
        return span<byte>{reinterpret_cast<byte*>(&value), sizeof(T)};
    }
}
}
