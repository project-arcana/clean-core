#pragma once

#include <cstdint>
#include <cstring>

#include <initializer_list>
#include <utility> // for tuple_size

#include <clean-core/allocator.hh>
#include <clean-core/always_false.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/container_impl_util.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash_combine.hh>
#include <clean-core/span.hh>

namespace cc
{
/// compile-time fixed-size array
template <class T, size_t N>
struct array
{
    static_assert(sizeof(T) > 0, "cannot make array of incomplete object");

    // must be public for ctor
    T _values[N];

    constexpr T* begin() { return _values; }
    constexpr T* end() { return _values + N; }
    constexpr T const* begin() const { return _values; }
    constexpr T const* end() const { return _values + N; }

    constexpr bool empty() const { return N > 0; }

    constexpr size_t size() const { return N; }
    constexpr size_t size_bytes() const { return N * sizeof(T); }

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

    template <size_t I>
    constexpr T& get()
    {
        static_assert(I < N);
        return _values[I];
    }
    template <size_t I>
    constexpr T const& get() const
    {
        static_assert(I < N);
        return _values[I];
    }

    bool operator==(cc::span<T const> rhs) const noexcept
    {
        if (N != rhs.size())
            return false;
        for (size_t i = 0; i < N; ++i)
            if (!(_values[i] == rhs[i]))
                return false;
        return true;
    }

    bool operator!=(cc::span<T const> rhs) const noexcept
    {
        if (N != rhs.size())
            return true;
        for (size_t i = 0; i < N; ++i)
            if (_values[i] != rhs[i])
                return true;
        return false;
    }
};

/// heap-allocated (runtime) fixed-size array
/// value semantics
template <class T>
struct array<T, dynamic_size>
{
    array() { static_assert(sizeof(T) > 0, "cannot make array of incomplete object"); }

    explicit array(size_t size)
    {
        static_assert(sizeof(T) > 0, "cannot make array of incomplete object");

        _size = size;
        _data = _alloc(size);

        if constexpr (!std::is_trivially_constructible_v<T>)
        {
            for (size_t i = 0; i < size; ++i)
                new (placement_new, &this->_data[i]) T();
        }
        else
        {
            std::memset(_data, 0, _size * sizeof(T));
        }
    }

    [[nodiscard]] static array defaulted(size_t size) { return array(size); }

    [[nodiscard]] static array uninitialized(size_t size)
    {
        array a;
        a._size = size;
        a._data = a._alloc(a._size);
        return a;
    }

    [[nodiscard]] static array filled(size_t size, T const& value)
    {
        array a;
        a._size = size;
        a._data = a._alloc(a._size);
        detail::container_copy_construct_fill(value, size, a._data);
        return a;
    }

    void resize(size_t new_size, T const& value = {})
    {
        _destroy();
        _size = new_size;
        _data = _alloc(_size);
        detail::container_copy_construct_fill(value, _size, _data);
    }

    array(std::initializer_list<T> data)
    {
        _size = data.size();
        _data = _alloc(_size);
        detail::container_copy_construct_range<T>(data.begin(), _size, _data);
    }

    array(span<T const> data)
    {
        _size = data.size();
        _data = _alloc(_size);
        detail::container_copy_construct_range<T>(data.data(), _size, _data);
    }

    array(array const& a)
    {
        _size = a._size;
        _data = _alloc(_size);
        detail::container_copy_construct_range<T>(a.data(), _size, _data);
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
            _destroy();
            _size = a._size;
            _data = _alloc(_size);
            detail::container_copy_construct_range<T>(a.data(), _size, _data);
        }
        return *this;
    }
    array& operator=(array&& a) noexcept
    {
        _destroy();
        _data = a._data;
        _size = a._size;
        a._data = nullptr;
        a._size = 0;
        return *this;
    }
    ~array()
    {
        static_assert(sizeof(T) > 0, "array destructor requires complete type");
        _destroy();
    }

    constexpr T* begin() { return _data; }
    constexpr T* end() { return _data + _size; }
    constexpr T const* begin() const { return _data; }
    constexpr T const* end() const { return _data + _size; }
    constexpr T* data() { return _data; }
    constexpr T const* data() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr size_t size_bytes() const { return _size * sizeof(T); }
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

    bool operator==(cc::span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return false;
        for (size_t i = 0; i < _size; ++i)
            if (!(_data[i] == rhs[i]))
                return false;
        return true;
    }

    bool operator!=(cc::span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return true;
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] != rhs[i])
                return true;
        return false;
    }

private:
    T* _alloc(size_t size) { return reinterpret_cast<T*>(cc::system_allocator->alloc(size * sizeof(T), alignof(T))); }
    void _free(T* p) { cc::system_allocator->free(p); }

    void _destroy()
    {
        detail::container_destroy_reverse<T>(_data, _size);
        _free(_data);
    }

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

// hash
template <class T, size_t N>
struct hash<array<T, N>>
{
    [[nodiscard]] constexpr uint64_t operator()(array<T, N> const& a) const noexcept
    {
        uint64_t h = 0;
        for (auto const& v : a)
            h = cc::hash_combine(h, hash<T>{}(v));
        return h;
    }
};
}

namespace std
{
template <class T, size_t N>
struct tuple_size<cc::array<T, N>> : public std::integral_constant<std::size_t, N>
{
};
template <class T>
struct tuple_size<cc::array<T, cc::dynamic_size>>
{
    static_assert(cc::always_false<T>, "does not work with dynamically sized arrays");
};
template <std::size_t I, class T, size_t N>
struct tuple_element<I, cc::array<T, N>>
{
    using type = T;
};
}
