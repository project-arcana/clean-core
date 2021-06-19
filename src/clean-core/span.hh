#pragma once

#include <cstddef>
#include <cstring> // memcpy

#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/is_contiguous_range.hh>
#include <clean-core/utility.hh>

namespace cc
{
/// a non-owning view of a contiguous array of Ts
/// can be read and write (span<const T> vs span<T>)
/// is trivially copyable (and cheap)
/// NOTE: is range-checked via CC_CONTRACT
template <class T>
struct span
{
    // ctors
public:
    constexpr span() = default;

    CC_FORCE_INLINE constexpr span(T* data, size_t size) : _data(data), _size(size) {}
    CC_FORCE_INLINE constexpr span(T* d_begin, T* d_end) : _data(d_begin), _size(d_end - d_begin) {}

    template <size_t N>
    CC_FORCE_INLINE constexpr span(T (&data)[N]) : _data(data), _size(N)
    {
    }

    /// generic span constructor from contiguous_range
    /// CAUTION: container MUST outlive the span!
    template <class Container, cc::enable_if<is_contiguous_range<Container, T> && !std::is_same_v<std::decay_t<Container>, span>> = true>
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

    span<std::byte const> as_bytes() const { return {reinterpret_cast<std::byte const*>(_data), _size * sizeof(T)}; }
    template <class U = T, cc::enable_if<!std::is_const_v<U>> = true>
    span<std::byte> as_writable_bytes() const
    {
        return {reinterpret_cast<std::byte*>(_data), _size * sizeof(T)};
    }

    template <class U>
    span<U> reinterpret_as() const
    {
        static_assert(std::is_trivially_copyable_v<T>, "only works if source type is trivially copyable");
        static_assert(std::is_trivially_copyable_v<U>, "only works if target type is trivially copyable");
        static_assert(std::is_const_v<U> || !std::is_const_v<T>, "cannot break const-correctness with reinterpret_as<U>");

        CC_CONTRACT(size_bytes() % sizeof(U) == 0 && "target size must be integer");
        return {reinterpret_cast<U*>(_data), _size * sizeof(T) / sizeof(U)};
    }

    // operations
public:
    /// copies all elements from the source to this span
    /// NOTE: sizes must match
    /// NOTE: target and this must not overlap!
    /// NOTE: uses std::memcpy for trivially copyable types
    /// TODO: versions with offset + count?
    template <class U = T const, enable_if<std::is_assignable_v<T&, U>> = true>
    constexpr void copy_from(span<U> source) const
    {
        CC_CONTRACT(source.size() == _size);

        if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<U>> && std::is_trivially_copyable_v<T>)
        {
            if (_size > 0)
                std::memcpy(_data, source._data, size_bytes());
        }
        else
        {
            for (size_t i = 0; i < _size; ++i)
                _data[i] = source._data[i];
        }
    }
    constexpr void copy_from(span<T const> source) const { this->copy_from<T const>(source); }

    /// copies all elements from this span to a target
    template <class U = std::remove_const_t<T>, enable_if<std::is_assignable_v<U&, T>> = true>
    constexpr void copy_to(span<U> target) const
    {
        target.copy_from(*this);
    }
    constexpr void copy_to(span<std::remove_const_t<T>> target) const { target.copy_from(*this); }

    /// fills this span with elements from the source up until capacity
    /// returns amount of elements in source
    constexpr size_t fill_from(span<T const> source) const
    {
        size_t const num_elements_written = cc::min(source._size, _size);
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            if (num_elements_written > 0)
            {
                std::memcpy(_data, source._data, num_elements_written * sizeof(T));
            }
        }
        else
        {
            for (size_t i = 0; i < num_elements_written; ++i)
            {
                _data[i] = source._data[i];
            }
        }

        return source._size;
    }

    // allow access to private fields across template instantiations
    template <class U>
    friend struct span;

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
        return span<std::byte>{reinterpret_cast<std::byte*>(value.data()), sizeof(ElementT) * value.size()};
    }
    else if constexpr (is_contiguous_range<T, void const>)
    {
        // container of some type
        using ElementT = std::remove_reference_t<decltype(value.data()[0])>;
        static_assert(std::is_trivially_copyable_v<ElementT>, "cannot convert range of non-trivially copyable elements to byte span");
        return span<std::byte const>{reinterpret_cast<std::byte const*>(value.data()), sizeof(ElementT) * value.size()};
    }
    else if constexpr (std::is_const_v<std::remove_reference_t<T>>)
    {
        // single POD type
        static_assert(std::is_trivially_copyable_v<std::remove_reference_t<T>>, "cannot convert non-trivially copyable element to byte span");
        return span<std::byte const>{reinterpret_cast<std::byte const*>(&value), sizeof(T)};
    }
    else
    {
        // single POD type
        static_assert(std::is_trivially_copyable_v<std::remove_reference_t<T>>, "cannot convert non-trivially copyable element to byte span");
        return span<std::byte>{reinterpret_cast<std::byte*>(&value), sizeof(T)};
    }
}

/// same as as_byte_span(value).subspan(offset, count)
template <class T>
auto as_byte_span(T&& value, size_t offset, size_t count)
{
    return cc::as_byte_span(value).subspan(offset, count);
}

/// reinterprets the given bytes as object T
template <class T>
T& from_byte_span(cc::span<std::byte> bytes)
{
    static_assert(std::is_trivially_copyable_v<T>);
    CC_ASSERT(bytes.size() == sizeof(T) && "size must match exactly");
    return *reinterpret_cast<T*>(bytes.data());
}
template <class T>
T& from_byte_span(cc::span<std::byte const> bytes)
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_const_v<T>, "cannot break const-correctness");
    CC_ASSERT(bytes.size() == sizeof(T) && "size must match exactly");
    return *reinterpret_cast<T*>(bytes.data());
}
}
