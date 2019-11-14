#pragma once

#include <initializer_list>

#include <clean-core/algorithms.hh>
#include <clean-core/assert.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/span.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
// compile-time fixed-size array
template <class T, size_t N>
struct array
{
    // must be public for ctor
    T _values[N];

    constexpr T* begin() { return _values; }
    constexpr T* end() { return _values + N; }
    constexpr T const* begin() const { return _values; }
    constexpr T const* end() const { return _values + N; }

    constexpr size_t size() const { return N; }

    constexpr T* data() { return _values; }
    constexpr T const* data() const { return _values; }

    constexpr T& operator[](size_t i)
    {
        CC_CONTRACT(i < N);
        return _values[i];
    }
    constexpr T const& operator[](size_t i) const
    {
        CC_CONTRACT(i < N);
        return _values[i];
    }

    constexpr bool operator==(array const& rhs) const { return cc::are_ranges_equal(*this, rhs); }
    constexpr bool operator!=(array const& rhs) const { return cc::are_ranges_unequal(*this, rhs); }
};

// heap-allocated (runtime) fixed-size array
// value semantics
template <class T>
struct array<T, dynamic_size>
{
    array() = default;

    explicit array(size_t size)
    {
        _size = size;
        _data = new T[_size](); // default ctor!
    }

    [[nodiscard]] static array defaulted(size_t size) { return array(size); }

    [[nodiscard]] static array uninitialized(size_t size)
    {
        array a;
        a._size = size;
        a._data = new T[size];
        return a;
    }

    [[nodiscard]] static array filled(size_t size, T const& value)
    {
        array a;
        a._size = size;
        a._data = new T[size];
        cc::fill(a, value);
        return a;
    }

    array(std::initializer_list<T> data)
    {
        _size = data.size();
        _data = new T[_size];
        cc::copy(data, *this);
    }

    array(span<T> data)
    {
        _size = data.size();
        _data = new T[_size];
        cc::copy(data, *this);
    }

    array(array const& a)
    {
        _size = a._size;
        _data = new T[a._size];
        cc::copy(a, *this);
    }
    array(array&& a) noexcept
    {
        _data = a._data;
        _size = a._size;
        a._data = nullptr;
        a._size = 0;
    }
    array& operator=(array const& a)
    {
        if (&a != this)
        {
            delete[] _data;
            _size = a._size;
            _data = new T[a._size];
            cc::copy(a, *this);
        }
        return *this;
    }
    array& operator=(array&& a) noexcept
    {
        _data = a._data;
        _size = a._size;
        a._data = nullptr;
        a._size = 0;
        return *this;
    }
    ~array() { delete[] _data; }

    constexpr T* begin() { return _data; }
    constexpr T* end() { return _data + _size; }
    constexpr T const* begin() const { return _data; }
    constexpr T const* end() const { return _data + _size; }
    constexpr T* data() { return _data; }
    constexpr T const* data() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr bool empty() const { return _size == 0; }

    constexpr T& operator[](size_t i)
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }
    constexpr T const& operator[](size_t i) const
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }

    constexpr bool operator==(array const& rhs) const { return cc::are_ranges_equal(*this, rhs); }
    constexpr bool operator!=(array const& rhs) const { return cc::are_ranges_unequal(*this, rhs); }

private:
    T* _data = nullptr;
    size_t _size = 0;
};

// deduction guides
template <class T, class... U>
array(T, U...) -> array<T, 1 + sizeof...(U)>;

template <class T, class... Args>
[[nodiscard]] array<T, 1 + sizeof...(Args)> make_array(T&& v0, Args&&... rest)
{
    return {{cc::forward<T>(v0), cc::forward<Args>(rest)...}};
}
}
