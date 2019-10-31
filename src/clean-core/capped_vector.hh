#pragma once

#include <clean-core/assert.hh>
#include <clean-core/detail/compact_size_t.hh>
#include <clean-core/forward.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>
#include <clean-core/storage.hh>

namespace cc
{
/// stack allocated vector type with compile time memory footprint but runtime size and vector interface
template <class T, size_t N>
struct capped_vector
{
    // properties
public:
    constexpr T* begin() { return &_u.value[0]; }
    constexpr T const* begin() const { return &_u.value[0]; }
    constexpr T* end() { return &_u.value[0] + _size; }
    constexpr T const* end() const { return &_u.value[0] + _size; }

    constexpr size_t size() const { return _size; }
    constexpr size_t capacity() const { return N; }
    constexpr bool empty() const { return _size == 0; }

    constexpr T* data() { return &_u.value[0]; }
    constexpr T const* data() const { return &_u.value[0]; }

    constexpr T& front()
    {
        CC_CONTRACT(_size > 0);
        return _u.value[0];
    }
    constexpr T const& front() const
    {
        CC_CONTRACT(_size > 0);
        return _u.value[0];
    }

    constexpr T& back()
    {
        CC_CONTRACT(_size > 0);
        return _u.value[_size - 1];
    }
    constexpr T const& back() const
    {
        CC_CONTRACT(_size > 0);
        return _u.value[_size - 1];
    }

    constexpr T const& operator[](size_t pos) const
    {
        CC_CONTRACT(pos < _size);
        return _u.value[pos];
    }

    constexpr T& operator[](size_t pos)
    {
        CC_CONTRACT(pos < _size);
        return _u.value[pos];
    }

    // ctors
public:
    constexpr capped_vector() = default;

    [[nodiscard]] static capped_vector defaulted(size_t size)
    {
        capped_vector cv;
        cv.resize(size, T());
        return cv;
    }

    [[nodiscard]] static capped_vector uninitialized(size_t size)
    {
        capped_vector cv;
        cv._size = size;
        return cv;
    }

    [[nodiscard]] static capped_vector filled(size_t size, T const& value)
    {
        capped_vector cv;
        cv.resize(size, value);
        return cv;
    }

    capped_vector(capped_vector const& rhs)
    {
        _size = rhs._size;
        for (size_t i = 0; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(rhs._u.value[i]);
    }
    capped_vector(capped_vector&& rhs) noexcept
    {
        _size = rhs._size;
        for (size_t i = 0; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(cc::move(rhs._u.value[i]));
        rhs._size = 0;
    }

    capped_vector& operator=(capped_vector const& rhs)
    {
        auto common_size = _size < rhs._size ? _size : rhs._size;

        // destroy superfluous entries
        for (size_t i = _size; i > rhs._size; --i)
            _u.value[i - 1].~T();

        _size = rhs._size;

        // copy assignment for common
        for (size_t i = 0; i < common_size; ++i)
            _u.value[i] = rhs._u.value[i];

        // copy ctor for new
        for (size_t i = common_size; i < _size; ++i)
            new (placement_new, &_u.value[i]) T(rhs._u.value[i]);

        return *this;
    }
    capped_vector& operator=(capped_vector&& rhs) noexcept
    {
        auto common_size = _size < rhs._size ? _size : rhs._size;

        // destroy superfluous entries
        for (size_t i = _size; i > rhs._size; --i)
            _u.value[i - 1].~T();

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

    ~capped_vector()
    {
        // deconstruct in reverse order
        for (size_t i = _size; i > 0; --i)
            _u.value[i - 1].~T();
    }

    // methods
public:
    void push_back(T const& t)
    {
        CC_CONTRACT(_size < N);
        new (placement_new, &_u.value[_size]) T(t);
        ++_size;
    }

    void push_back(T&& t)
    {
        CC_CONTRACT(_size < N);
        new (placement_new, &_u.value[_size]) T(cc::move(t));
        ++_size;
    }

    void pop_back()
    {
        CC_CONTRACT(_size > 0);
        --_size;
        _u.value[_size].~T();
    }

    template <class... Args>
    T& emplace_back(Args&&... args)
    {
        CC_CONTRACT(_size < N);
        new (placement_new, &_u.value[_size]) T(cc::forward<Args>(args)...);
        ++_size;
        return _u.value[_size - 1];
    }

    void clear()
    {
        // deconstruct in reverse order
        for (size_t i = _size; i > 0; --i)
            _u.value[i - 1].~T();
        _size = 0;
    }

    void resize(size_t new_size)
    {
        CC_CONTRACT(new_size <= N);
        for (size_t i = _size; i < new_size; ++i)
            new (placement_new, &_u.value[i]) T();
        for (size_t i = _size; i > new_size; --i)
            _u.value[i - 1].~T();
        _size = compact_size_t(new_size);
    }

    void resize(size_t new_size, T const& default_value)
    {
        CC_CONTRACT(new_size <= N);
        for (size_t i = _size; i < new_size; ++i)
            new (placement_new, &_u.value[i]) T(default_value);
        for (size_t i = _size; i > new_size; --i)
            _u.value[i - 1].~T();
        _size = compact_size_t(new_size);
    }

    template <size_t M>
    constexpr bool operator==(capped_vector<T, M> const& rhs) const noexcept
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

    template <size_t M>
    constexpr bool operator!=(capped_vector<T, M> const& rhs) const noexcept
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
    using compact_size_t = detail::compact_size_t_typed<T, N>;

    compact_size_t _size = 0;
    storage_for<T[N]> _u;
};
}
