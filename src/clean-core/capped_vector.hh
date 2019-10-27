#pragma once

#include <clean-core/forward.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/compact_size_t.hh>

namespace cc
{
/// stack allocated vector type with compile time memory footprint but runtime size and vector interface
template <class T, size_t N>
struct capped_vector
{
    using compact_size_t = detail::compact_size_t_for<N, alignof(T)>;

    constexpr capped_vector() = default;

    ~capped_vector()
    {
        // deconstruct in reverse order
        for (size_t i = _size; i > 0; --i)
            _u._data[i - 1].~T();
    }

    T const& operator[](size_t pos) const
    {
        CC_CONTRACT(pos < _size);
        return _u._data[pos];
    }

    T& operator[](size_t pos)
    {
        CC_CONTRACT(pos < _size);
        return _u._data[pos];
    }

    void push_back(T const& t)
    {
        CC_CONTRACT(_size < N);
        new (&_u._data[_size]) T(t);
        ++_size;
    }

    void push_back(T&& t)
    {
        CC_CONTRACT(_size < N);
        new (&_u._data[_size]) T(move(t));
        ++_size;
    }

    void pop_back()
    {
        CC_CONTRACT(_size > 0);
        --_size;
        _u._data[_size].~T();
    }

    template <typename... Args>
    T& emplace_back(Args&&... args)
    {
        CC_CONTRACT(_size < N);
        new (&_u._data[_size]) T(cc::forward<Args>(args)...);
        ++_size;
        return _u._data[_size - 1];
    }

    void clear()
    {
        // deconstruct in reverse order
        for (size_t i = _size; i > 0; --i)
            _u._data[i - 1].~T();
        _size = 0;
    }

    void resize(size_t new_size)
    {
        CC_CONTRACT(new_size <= N);
        for (size_t i = _size; i < new_size; ++i)
            new (&_u._data[i]) T();
        for (size_t i = _size; i > new_size; --i)
            _u._data[i - 1].~T();
        _size = compact_size_t(new_size);
    }

    constexpr T& front()
    {
        CC_CONTRACT(_size > 0);
        return _u._data[0];
    }

    constexpr T const& front() const
    {
        CC_CONTRACT(_size > 0);
        return _u._data[0];
    }

    constexpr T& back()
    {
        CC_CONTRACT(_size > 0);
        return _u._data[_size - 1];
    }

    constexpr T const& back() const
    {
        CC_CONTRACT(_size > 0);
        return _u._data[_size - 1];
    }

    constexpr T* begin() { return &_u._data[0]; }
    constexpr T const* begin() const { return &_u._data[0]; }
    constexpr T* end() { return &_u._data[0] + _size; }
    constexpr T const* end() const { return &_u._data[0] + _size; }

    constexpr size_t size() const { return _size; }
    constexpr size_t capacity() const { return N; }
    constexpr bool empty() const { return _size == 0; }

    constexpr T* data() { return &_u._data[0]; }
    constexpr T const* data() const { return &_u._data[0]; }

    template <int M>
    constexpr bool operator==(capped_vector<T, M> const& rhs) const
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
    constexpr bool operator!=(capped_vector<T, M> const& rhs) const
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
    /// uninitialized memory
    union DataHolder {
        DataHolder() {}
        ~DataHolder() {}
        T _data[N];
    };
    compact_size_t _size = 0;
    DataHolder _u;
};
}
