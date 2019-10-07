#pragma once

#include <cc/always_false.hh>
#include <cc/assert.hh>
#include <cc/fwd.hh>

#include <utility>

namespace cc
{
struct nullopt_t
{
};
constexpr nullopt_t nullopt;

template <class T>
struct optional
{
    constexpr optional() {}
    constexpr optional(nullopt_t) noexcept {}

    [[nodiscard]] constexpr bool has_value() const { return _has_value; }

    [[nodiscard]] constexpr T& value() &
    {
        CC_CONTRACT(!_has_value);
        return _data.v;
    }
    [[nodiscard]] constexpr T const& value() const&
    {
        CC_CONTRACT(!_has_value);
        return _data.v;
    }
    [[nodiscard]] constexpr T&& value() &&
    {
        CC_CONTRACT(!_has_value);
        return static_cast<T&&>(_data.v);
    }
    [[nodiscard]] constexpr T const&& value() const&&
    {
        CC_CONTRACT(!_has_value);
        return static_cast<T const&&>(_data.v);
    }

    template <class U>
    [[nodiscard]] constexpr T const& value_or(U&& default_value) const&
    {
        return _has_value ? _data.v : static_cast<T>(std::forward<U>(default_value));
    }
    template <class U>
    [[nodiscard]] constexpr T&& value_or(U&& default_value) &&
    {
        return _has_value ? static_cast<T&&>(_data.v) : static_cast<T>(std::forward<U>(default_value));
    }

    ~optional()
    {
        if (_has_value)
            _data.v.~T();
    }

    template <class U>
    constexpr bool operator==(U const& rhs) const
    {
        return _has_value ? _data.v == rhs : false;
    }
    template <class U>
    constexpr bool operator!=(U const& rhs) const
    {
        return _has_value ? _data.v != rhs : true;
    }
    template <class U>
    constexpr bool operator==(optional<U> const& rhs) const
    {
        if (_has_value && rhs._has_value)
            return _data.v == rhs._data.v;
        return _has_value == rhs._has_value;
    }
    template <class U>
    constexpr bool operator!=(optional<U> const& rhs) const
    {
        if (_has_value && rhs._has_value)
            return _data.v != rhs._data.v;
        return _has_value != rhs._has_value;
    }

private:
    struct empty
    {
    };
    union {
        empty e;
        T v;
    } _data = {};
    bool _has_value = false;
};

template <class T>
struct optional<T&>
{
    static_assert(always_false<T>, "cannot form optional reference");
};
template <class T>
struct optional<T&&>
{
    static_assert(always_false<T>, "cannot form optional reference");
};

// ========== hash and less ==========
template <class T>
struct less<optional<T>>
{
    [[nodiscard]] bool operator()(optional<T> const& a, optional<T> const& b) const noexcept
    {
        if (!a.has_value() && !b.has_value())
            return false;
        if (!a.has_value())
            return true;
        if (!b.has_value())
            return false;
        return less<T>{}();
    }
};
}
