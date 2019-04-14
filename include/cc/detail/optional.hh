#pragma once

#include <cc/assert>
#include <cc/fwd/hash.hh>
#include <cc/fwd/less.hh>
#include <cc/fwd/optional.hh>
#include <cc/move>

namespace cc
{
struct nullopt_t
{
};
inline constexpr nullopt_t nullopt;

template <class T>
struct optional
{
    constexpr optional() {}
    constexpr optional(nullopt_t) noexcept {}

    [[nodiscard]] constexpr bool has_value() const { return _has_value; }

    [[nodiscard]] constexpr T& value() &
    {
        CC_ASSERT_CONTRACT(!_has_value);
        return _data.v;
    }
    [[nodiscard]] constexpr T const& value() const&
    {
        CC_ASSERT_CONTRACT(!_has_value);
        return _data.v;
    }
    [[nodiscard]] constexpr T&& value() &&
    {
        CC_ASSERT_CONTRACT(!_has_value);
        return static_cast<T&&>(_data.v);
    }
    [[nodiscard]] constexpr T const&& value() const&&
    {
        CC_ASSERT_CONTRACT(!_has_value);
        return static_cast<T const&&>(_data.v);
    }

    template <class U>
    [[nodiscard]] constexpr T const& value_or(U&& default_value) const&
    {
        return _has_value ? _data.v : static_cast<T>(forward<U>(default_value));
    }
    template <class U>
    [[nodiscard]] constexpr T&& value_or(U&& default_value) &&
    {
        return _has_value ? static_cast<T&&>(_data.v) : static_cast<T>(forward<U>(default_value));
    }

    ~optional()
    {
        if (_has_value)
            _data.v.~T();
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
    static_assert(sizeof(T) >= 0, "cannot form optional reference");
};
template <class T>
struct optional<T&&>
{
    static_assert(sizeof(T) >= 0, "cannot form optional reference");
};
} // namespace cc
