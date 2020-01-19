#pragma once

#include <cstring>
#include <type_traits>

#include <clean-core/algorithms.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/compact_size_t.hh>
#include <clean-core/detail/container_impl_util.hh>
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
    constexpr size_t max_size() const { return N; }
    constexpr bool empty() const { return _size == 0; }

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
        CC_CONTRACT(size <= N);
        capped_array a;
        a._size = static_cast<compact_size_t>(size);
        new (&a._u.value[0]) T[size];
        return a;
    }

    [[nodiscard]] static capped_array filled(size_t size, T const& value)
    {
        CC_CONTRACT(size <= N);
        capped_array a;
        a._size = static_cast<compact_size_t>(size);
        for (compact_size_t i = 0; i < size; ++i)
            new (placement_new, &a._u.value[i]) T(value);
        return a;
    }

    template <class... Args>
    void emplace(size_t new_size, Args&&... init_args)
    {
        CC_CONTRACT(new_size <= N);

        // destroy current contents
        detail::container_destroy_reverse<T, compact_size_t>(_u.value, _size);

        _size = static_cast<compact_size_t>(new_size);
        for (compact_size_t i = 0; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(init_args...);
    }

    capped_array(capped_array const& rhs) : _size(rhs._size)
    {
        detail::container_copy_range<T, compact_size_t>(&rhs._u.value[0], _size, &_u.value[0]);
    }
    capped_array(capped_array&& rhs) noexcept : _size(rhs._size)
    {
        detail::container_move_range<T, compact_size_t>(&rhs._u.value[0], _size, &_u.value[0]);
        rhs._size = 0;
    }

    capped_array& operator=(capped_array const& rhs)
    {
        auto common_size = _size < rhs._size ? _size : rhs._size;

        // destroy superfluous entries
        detail::container_destroy_reverse<T, compact_size_t>(&_u.value[0], _size, rhs._size);

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
        detail::container_destroy_reverse<T, compact_size_t>(_u.value, _size, rhs._size);

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

    ~capped_array() { detail::container_destroy_reverse<T, compact_size_t>(_u.value, _size); }

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
