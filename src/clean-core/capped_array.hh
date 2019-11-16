#pragma once

#include <cstring>
#include <type_traits>

#include <clean-core/algorithms.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/compact_size_t.hh>
#include <clean-core/forward.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>
#include <clean-core/storage.hh>

namespace cc
{
/// Array type with compile time memory footprint but runtime size
template <class T, size_t N>
struct capped_array
{
    // properties
public:
    constexpr T* begin() { return &_u.value[0]; }
    constexpr T const* begin() const { return &_u.value[0]; }
    constexpr T* end() { return &_u.value[0] + _size; }
    constexpr T const* end() const { return &_u.value[0] + _size; }

    constexpr size_t size() const { return _size; }

    constexpr T* data() { return &_u.value[0]; }
    constexpr T const* data() const { return &_u.value[0]; }

    const T& operator[](size_t pos) const
    {
        CC_CONTRACT(pos < _size);
        return _u.value[pos];
    }

    T& operator[](size_t pos)
    {
        CC_CONTRACT(pos < _size);
        return _u.value[pos];
    }

    // ctors
public:
    using compact_size_t = detail::compact_size_t_typed<T, N>;
    capped_array() = default;

    constexpr capped_array(size_t size) : _size(compact_size_t(size))
    {
        CC_CONTRACT(size <= N);
        new (&_u.value[0]) T[size]();
    }

    [[nodiscard]] static capped_array defaulted(size_t size) { return capped_array(size); }

    [[nodiscard]] static capped_array uninitialized(size_t size)
    {
        capped_array a;
        a._size = size;
        new (&a._u.value[0]) T[size];
        return a;
    }

    [[nodiscard]] static capped_array filled(size_t size, T const& value)
    {
        capped_array a;
        a._size = size;
        for (compact_size_t i = 0; i < size; ++i)
            new (placement_new, &a._u.value[i]) T(value);
        return a;
    }

    template <class... Args>
    void emplace(compact_size_t new_size, Args&&... init_args)
    {
        CC_CONTRACT(new_size < N);

        // destroy current contents
        _destroy_reverse(_u.value, _size);

        _size = new_size;
        for (compact_size_t i = 0; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(init_args...);
    }

    capped_array(capped_array const& rhs) : _size(rhs.size) { _copy_range(rhs._u.value, _size, _u.value); }
    capped_array(capped_array&& rhs) noexcept : _size(rhs.size) { _move_range(rhs._u.value, _size, _u.value); }

    capped_array& operator=(capped_array const& rhs)
    {
        auto common_size = _size < rhs._size ? _size : rhs._size;

        // destroy superfluous entries
        _destroy_reverse(_u.value, _size, rhs._size);

        _size = rhs._size;

        // copy assignment for common
        for (size_t i = 0; i < common_size; ++i)
            _u.value[i] = rhs._u.value[i];

        // copy ctor for new
        for (size_t i = common_size; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(rhs._u.value[i]);

        return *this;
    }
    capped_array& operator=(capped_array&& rhs) noexcept
    {
        auto common_size = _size < rhs._size ? _size : rhs._size;

        // destroy superfluous entries
        _destroy_reverse(_u.value, _size, rhs._size);

        _size = rhs._size;

        // move assignment for common
        for (size_t i = 0; i < common_size; ++i)
            _u.value[i] = cc::move(rhs._u.value[i]);

        // movector for new
        for (size_t i = common_size; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(cc::move(rhs._u.value[i]));

        rhs._size = 0;

        return *this;
    }

    ~capped_array() { _destroy_reverse(_u.value, _size); }

    // methods
public:
    template <int M>
    constexpr bool operator==(capped_array<T, M> const& rhs) const noexcept
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
    constexpr bool operator!=(capped_array<T, M> const& rhs) const noexcept
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

    // members
private:
    static void _move_range(T* src, compact_size_t num, T* dest)
    {
        for (compact_size_t i = 0; i < num; ++i)
            new (placement_new, &dest[i]) T(cc::move(src[i]));
    }
    static void _copy_range(T* src, compact_size_t num, T* dest)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(dest, src, sizeof(T) * num);
        }
        else
        {
            for (compact_size_t i = 0; i < num; ++i)
                new (placement_new, &dest[i]) T(src[i]);
        }
    }
    static void _destroy_reverse(T* data, compact_size_t size, compact_size_t to_index = 0)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (compact_size_t i = size; i > to_index; --i)
                data[i - 1].~T();
        }
    }


    compact_size_t _size = 0;
    storage_for<T[N]> _u;
};

template <class T>
struct capped_array<T, 0>
{
    // properties
public:
    constexpr T* begin() { return nullptr; }
    constexpr T const* begin() const { return nullptr; }
    constexpr T* end() { return nullptr; }
    constexpr T const* end() const { return nullptr; }

    constexpr size_t size() const { return 0; }

    constexpr T* data() { return nullptr; }
    constexpr T const* data() const { return nullptr; }

    // ctors
public:
    constexpr capped_array() = default;

    template <size_t M>
    constexpr bool operator==(capped_array<T, M> const& rhs) const noexcept
    {
        return (rhs._size == 0);
    }

    template <size_t M>
    constexpr bool operator!=(capped_array<T, M> const& rhs) const noexcept
    {
        return (rhs._size != 0);
    }
};
}
