#pragma once

#include <clean-core/algorithms.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/compact_size_t.hh>
#include <clean-core/storage.hh>

namespace cc
{
/// Array type with compile time memory footprint but runtime size
template <class T, size_t N>
struct capped_array
{
    using compact_size_t = detail::compact_size_t_for<N, alignof(T)>;

    constexpr capped_array(size_t size) : _size(compact_size_t(size))
    {
        CC_CONTRACT(size <= N);
        new (&_u._data[0]) T[size]();
    }

    [[nodiscard]] static capped_array defaulted(size_t size) { return capped_array(size); }

    [[nodiscard]] static capped_array uninitialized(size_t size)
    {
        capped_array a;
        a._size = size;
        return a;
    }

    [[nodiscard]] static capped_array filled(size_t size, T const& value)
    {
        capped_array a;
        a._size = size;
        a._data = new T[size];
        cc::fill(a, value);
        return a;
    }

    ~capped_array()
    {
        for (size_t i = 0; i < _size; ++i)
            _u._data[i].~T();
    }

    const T& operator[](size_t pos) const
    {
        CC_CONTRACT(pos < _size);
        return _u._data[pos];
    }

    T& operator[](size_t pos)
    {
        CC_CONTRACT(pos < _size);
        return _u._data[pos];
    }

    constexpr T* begin() { return &_u._data[0]; }
    constexpr T const* begin() const { return &_u._data[0]; }
    constexpr T* end() { return &_u._data[0] + _size; }
    constexpr T const* end() const { return &_u._data[0] + _size; }

    constexpr size_t size() const { return _size; }

    constexpr T* data() { return &_u._data[0]; }
    constexpr T const* data() const { return &_u._data[0]; }

    template <int M>
    constexpr bool operator==(capped_array<T, M> const& rhs) const
    {
        if (_size != rhs._size)
            return false;
        for (size_t i = 0; i < _size; ++i)
        {
            if ((*this)[i] != rhs[i])
                return false;
        }
        return true;
    }

    template <int M>
    constexpr bool operator!=(capped_array<T, M> const& rhs) const
    {
        if (_size != rhs._size)
            return true;
        for (size_t i = 0; i < _size; ++i)
        {
            if ((*this)[i] != rhs[i])
                return true;
        }
        return false;
    }

private:
    compact_size_t _size = 0;
    storage_for<T[N]> _u;
};
}
