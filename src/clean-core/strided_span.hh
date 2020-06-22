#pragma once

#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/fwd.hh>
#include <clean-core/is_contiguous_range.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/typedefs.hh>

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

    // ctors
public:
    constexpr strided_span() = default;
    constexpr strided_span(T* data, size_t size, int64 stride = sizeof(T)) : _data(reinterpret_cast<byte_t*>(data)), _size(size), _stride(stride) {}
    constexpr strided_span(T* d_begin, T* d_end) : strided_span(d_begin, d_end - d_begin) {}
    template <size_t N>
    constexpr strided_span(T (&data)[N]) : strided_span(data, N)
    {
    }
    template <class Container, cc::enable_if<is_contiguous_range<Container, T>> = true>
    constexpr strided_span(Container& c) : strided_span(c.data(), c.size())
    {
    }

    explicit constexpr strided_span(T& val) : strided_span(&val, 1) {}

    constexpr operator strided_span<T const>() const noexcept { return {_data, _size, _stride}; }

    // container
public:
    constexpr byte_t* data() const { return _data; }
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

    // range
public:
    struct iterator
    {
        byte_t* ptr;
        size_t size;
        int64 stride;

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
    int64 _stride = 0;
};

// deduction guide for containers
template <class Container, cc::enable_if<is_contiguous_range<Container, void>> = true>
strided_span(Container& c)->strided_span<std::remove_reference_t<decltype(*c.data())>>;
strided_span(string_view const&)->strided_span<char const>;
}
