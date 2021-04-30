#pragma once

#include <type_traits>

#include <clean-core/always_false.hh>
#include <clean-core/assert.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>
#include <clean-core/storage.hh>

namespace cc
{
struct nullopt_t
{
    // NOTE: no default ctor must be declared
    //       otherwise optional<T> = {} does not work
    //       (due to ambiguous overload)
    enum class _ctor_tag
    {
        tag
    };
    explicit constexpr nullopt_t(_ctor_tag) {}
};
constexpr nullopt_t nullopt = nullopt_t{nullopt_t::_ctor_tag::tag};

/// an optional value
///
/// TODO:
///   - conditional explicit ctors
///   - conditional triviality
template <class T>
struct optional
{
    // ctors / dtor / assignment
public:
    constexpr optional() {}
    constexpr optional(nullopt_t) {}

    optional(T&& v)
    {
        new (cc::placement_new, &_data.value) T(cc::move(v));
        _has_value = true;
    }
    optional(T const& v)
    {
        new (cc::placement_new, &_data.value) T(v);
        _has_value = true;
    }

    // NOTE: this complicated construction is necessary
    //       - !std::is_same_v<std::decay_t<U>, optional> prevents hijacking of op=(optional const&&)
    //       - std::is_assignable_v<T&, U> prevents compile error in optional<T> = optional<U>
    //       - !std::is_same_v<std::decay_t<U>, T> prevents wrong optional<T> = {}
    template <class U = T,
              cc::enable_if<!std::is_same_v<std::decay_t<U>, optional> && //
                            std::is_assignable_v<T&, U> &&                //
                            !std::is_same_v<std::decay_t<U>, T>> = true>
    optional& operator=(U&& v)
    {
        if (_has_value)
            _data.value.~T();
        new (cc::placement_new, &_data.value) T(cc::forward<U>(v));
        _has_value = true;
        return *this;
    }

    constexpr optional(optional&& rhs) noexcept
    {
        if (rhs._has_value)
        {
            new (cc::placement_new, &_data.value) T(cc::move(rhs._data.value));
            rhs._data.value.~T();
            rhs._has_value = false;
            _has_value = true;
        }
    }
    constexpr optional(optional const& rhs)
    {
        if (rhs._has_value)
        {
            new (cc::placement_new, &_data.value) T(rhs._data.value);
            _has_value = true;
        }
    }

    constexpr optional& operator=(nullopt_t)
    {
        if (_has_value)
        {
            _data.value.~T();
            _has_value = false;
        }
        return *this;
    }
    constexpr optional& operator=(optional&& rhs) noexcept
    {
        if (_has_value)
        {
            _data.value.~T();
            _has_value = false;
        }

        if (rhs._has_value)
        {
            new (cc::placement_new, &_data.value) T(cc::move(rhs._data.value));
            rhs._data.value.~T();
            rhs._has_value = false;
            _has_value = true;
        }

        return *this;
    }
    constexpr optional& operator=(optional const& rhs)
    {
        if (_has_value)
        {
            _data.value.~T();
            _has_value = false;
        }

        if (this != &rhs && rhs._has_value)
        {
            new (cc::placement_new, &_data.value) T(rhs._data.value);
            _has_value = true;
        }

        return *this;
    }

    ~optional()
    {
        if (_has_value)
            _data.value.~T();
    }

    // conversion
    template <class U, cc::enable_if<std::is_convertible_v<T, U>> = true>
    operator optional<U>()
    {
        optional<U> o;
        if (_has_value)
            o.emplace(_data.value);
        return o;
    }

    // value API
public:
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

    template <class U = T>
    [[nodiscard]] constexpr T const& value_or(U&& default_value) const&
    {
        return _has_value ? _data.value : static_cast<T const&>(cc::forward<U>(default_value));
    }
    template <class U = T>
    [[nodiscard]] constexpr T&& value_or(U&& default_value) &&
    {
        return _has_value ? static_cast<T&&>(_data.value) : static_cast<T&&>(cc::forward<U>(default_value));
    }

    /// creates a new optional value in-place
    template <class... Args>
    T& emplace(Args&&... args)
    {
        if (_has_value)
            _data.value.~T();
        new (cc::placement_new, &_data.value) T(cc::forward<Args>(args)...);
        _has_value = true;
        return _data.value;
    }

    // comparisons
public:
    constexpr bool operator==(nullopt_t) const { return !_has_value; }
    constexpr bool operator!=(nullopt_t) const { return _has_value; }

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

template <class T>
constexpr bool operator==(nullopt_t, optional<T> const& rhs)
{
    return !rhs.has_value();
}
template <class T>
constexpr bool operator!=(nullopt_t, optional<T> const& rhs)
{
    return rhs.has_value();
}

template <class U, class V>
constexpr bool operator==(optional<U> const& lhs, V const& rhs)
{
    return lhs.has_value() ? lhs.value() == rhs : false;
}
template <class U, class V>
constexpr bool operator!=(optional<U> const& lhs, V const& rhs)
{
    return lhs.has_value() ? lhs.value() != rhs : true;
}

template <class U, class V>
constexpr bool operator==(optional<U> const& lhs, optional<V> const& rhs)
{
    if (lhs.has_value() && rhs.has_value())
        return lhs.value() == rhs.value();
    return lhs.has_value() == rhs.has_value();
}
template <class U, class V>
constexpr bool operator!=(optional<U> const& lhs, optional<V> const& rhs)
{
    if (lhs.has_value() && rhs.has_value())
        return lhs.value() != rhs.value();
    return lhs.has_value() != rhs.has_value();
}

template <class U, class V>
constexpr bool operator==(U const& lhs, optional<V> const& rhs)
{
    return rhs.has_value() ? rhs.value() == lhs : false;
}
template <class U, class V>
constexpr bool operator!=(U const& lhs, optional<V> const& rhs)
{
    return rhs.has_value() ? rhs.value() != lhs : true;
}

/// wraps the given value in an optional
template <class T>
optional<std::decay_t<T>> make_optional(T&& value)
{
    return cc::optional<std::decay_t<T>>(cc::forward<T>(value));
}
/// construct an optional<T> in-place by passing args to the ctor of T
template <class T, class... Args>
optional<T> make_optional(Args&&... args)
{
    auto o = cc::optional<T>();
    o.template emplace<T>(cc::forward<Args>(args)...);
    return o;
}

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
        return less<T>{}(a.value(), b.value());
    }
};
template <class T>
struct hash<optional<T>>
{
    [[nodiscard]] constexpr uint64_t operator()(optional<T> const& v) const noexcept
    {
        if (v.has_value())
            return hash<T>{}(v.value());
        else
            return 0x7132ae00db5bc3bb; // random value for nullopt
    }
};
}
