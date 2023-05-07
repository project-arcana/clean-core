#pragma once

#include <cstddef>
#include <cstdint>

#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/fwd.hh>
#include <clean-core/is_contiguous_range.hh>
#include <clean-core/sentinel.hh>

namespace cc
{
// a non-owning view of a strided array of Ts
// can be read and write (strided_span<const T> vs strided_span<T>)
// is trivially copyable (and cheap)
// NOTE: is range-checked via CC_CONTRACT
//       stride is in bytes, NOT elements!
template <class T>
struct strided_span
{
    using byte_t = std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>;
    using mutable_t = std::remove_const_t<T>;

    // ctors
public:
    constexpr strided_span() = default;
    constexpr strided_span(T* data, size_t size, int64_t stride = sizeof(T)) : _data(reinterpret_cast<byte_t*>(data)), _size(size), _stride(stride) {}
    constexpr strided_span(T* d_begin, T* d_end) : strided_span(d_begin, d_end - d_begin) {}
    template <size_t N>
    constexpr strided_span(T (&data)[N]) : strided_span(data, N)
    {
    }

    /// generic span constructor from contiguous_range
    /// CAUTION: container MUST outlive the span!
    template <class Container, cc::enable_if<is_contiguous_range<Container, T>> = true>
    constexpr strided_span(Container&& c) : strided_span(c.data(), c.size())
    {
    }

    explicit constexpr strided_span(T& val) : strided_span(&val, 1) {}
    /// CAUTION: value MUST outlive the span!
    /// NOTE: this ctor is for spans constructed inside an expression
    explicit constexpr strided_span(T&& val) : strided_span(&val, 1) {}

    constexpr operator strided_span<T const>() const noexcept { return {reinterpret_cast<T const*>(_data), _size, _stride}; }

    // container
public:
    /// NOTE: this is puposely not data() so it's not confused with a contiguous range
    constexpr byte_t* data_ptr() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr size_t size_bytes() const { return _size * sizeof(T); }
    constexpr size_t stride() const { return _stride; }
    constexpr bool empty() const { return _size == 0; }

    constexpr T& operator[](size_t i) const
    {
        CC_CONTRACT(i < _size);
        return *reinterpret_cast<T*>(_data + _stride * i);
    }

    constexpr T& front() const
    {
        CC_CONTRACT(_size > 0);
        return *reinterpret_cast<T*>(_data);
    }
    constexpr T& back() const
    {
        CC_CONTRACT(_size > 0);
        return *reinterpret_cast<T*>(_data + _stride * (_size - 1));
    }

    // subviews
public:
    constexpr strided_span first(size_t n) const
    {
        CC_CONTRACT(n <= _size);
        return {reinterpret_cast<T*>(_data), n, _stride};
    }
    constexpr strided_span last(size_t n) const
    {
        CC_CONTRACT(n <= _size);
        return {reinterpret_cast<T*>(_data + _stride * (_size - n)), n, _stride};
    }
    constexpr strided_span subspan(size_t offset, size_t count) const
    {
        CC_CONTRACT(offset <= _size && offset + count <= _size);
        return {reinterpret_cast<T*>(_data + _stride * offset), count, _stride};
    }
    constexpr strided_span subspan(size_t offset) const
    {
        CC_CONTRACT(offset <= _size);
        return {reinterpret_cast<T*>(_data + _stride * offset), _size - offset, _stride};
    }
    constexpr strided_span reversed() const { return {reinterpret_cast<T*>(_data + _stride * (_size - 1)), _size, -_stride}; }

    /// returns a new strided span that
    template <class U, class TT = mutable_t, cc::enable_if<std::is_class_v<TT>> = true>
    auto project(U TT::*member) const -> strided_span<std::conditional_t<std::is_const_v<T>, U const, U>>
    {
        auto new_data = &(reinterpret_cast<T*>(_data)->*member);
        return {new_data, _size, _stride};
    }

    // range
public:
    struct iterator
    {
        byte_t* ptr;
        size_t size;
        int64_t stride;

        T& operator*()
        {
            CC_ASSERT(ptr && "no data");
            return *reinterpret_cast<T*>(ptr);
        }

        void operator++()
        {
            CC_ASSERT(size > 0 && "out of bounds");
            ptr += stride;
            size--;
        }

        bool operator!=(cc::sentinel) const { return size > 0; }
    };

    constexpr iterator begin() const { return {_data, _size, _stride}; }
    constexpr cc::sentinel end() const { return {}; }

private:
    byte_t* _data = nullptr;
    size_t _size = 0;
    int64_t _stride = 0;
};

// deduction guide for containers
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
strided_span(Container& c) -> strided_span<std::remove_reference_t<decltype(*c.data())>>;
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
strided_span(Container&& c) -> strided_span<std::remove_reference_t<decltype(*c.data())>>;
}
