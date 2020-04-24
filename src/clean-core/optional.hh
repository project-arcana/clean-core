#pragma once

#include <clean-core/always_false.hh>
#include <clean-core/assert.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/storage.hh>

namespace cc
{
struct nullopt_t
{
};
constexpr nullopt_t nullopt;

// NOTE: IS NOT FINISHED!
template <class T>
struct optional
{
    // missing:
    // assignment, move ops
    static_assert(always_false<T>, "NOT FINISHED!");

    constexpr optional() {}
    constexpr optional(nullopt_t) noexcept {}

    [[nodiscard]] constexpr bool has_value() const { return _has_value; }

    [[nodiscard]] constexpr T& value() &
    {
        CC_CONTRACT(_has_value);
        return _data.value;
    }
    [[nodiscard]] constexpr T const& value() const&
    {
        CC_CONTRACT(_has_value);
        return _data.value;
    }
    [[nodiscard]] constexpr T&& value() &&
    {
        CC_CONTRACT(_has_value);
        return static_cast<T&&>(_data.value);
    }
    [[nodiscard]] constexpr T const&& value() const&&
    {
        CC_CONTRACT(_has_value);
        return static_cast<T const&&>(_data.value);
    }

    template <class U>
    [[nodiscard]] constexpr T const& value_or(U&& default_value) const&
    {
        return _has_value ? _data.value : static_cast<T>(cc::forward<U>(default_value));
    }
    template <class U>
    [[nodiscard]] constexpr T&& value_or(U&& default_value) &&
    {
        return _has_value ? static_cast<T&&>(_data.value) : static_cast<T>(cc::forward<U>(default_value));
    }

    ~optional()
    {
        if (_has_value)
            _data.value.~T();
    }

    template <class U>
    constexpr bool operator==(U const& rhs) const
    {
        return _has_value ? _data.value == rhs : false;
    }
    template <class U>
    constexpr bool operator!=(U const& rhs) const
    {
        return _has_value ? _data.value != rhs : true;
    }
    template <class U>
    constexpr bool operator==(optional<U> const& rhs) const
    {
        if (_has_value && rhs._has_value)
            return _data.value == rhs._data.value;
        return _has_value == rhs._has_value;
    }
    template <class U>
    constexpr bool operator!=(optional<U> const& rhs) const
    {
        if (_has_value && rhs._has_value)
            return _data.value != rhs._data.value;
        return _has_value != rhs._has_value;
    }
    template <class U>
    friend constexpr bool operator==(U const& lhs, optional const& rhs)
    {
        return rhs._has_value ? rhs._data.value == lhs : false;
    }
    template <class U>
    friend constexpr bool operator!=(U const& lhs, optional const& rhs)
    {
        return rhs._has_value ? rhs._data.value != lhs : true;
    }

private:
    storage_for<T> _data;
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
