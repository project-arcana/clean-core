#pragma once

#include <cstdint>
#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/has_operator.hh>
#include <clean-core/new.hh>

namespace cc
{
namespace detail
{
template <class T, class... Types>
using first_of = T;

template <class T, class... Types>
static constexpr bool is_equal_any_of = (std::is_same_v<T, Types> || ...);

template <class T, class... Types>
constexpr int index_of_type()
{
    int i = -1;
    ((++i, std::is_same_v<T, Types>) || ...);
    return i == int(sizeof...(Types)) ? -1 : i;
}

template <class... Types>
struct variant_impl;
template <>
struct variant_impl<>
{
};
template <class T, class... Rest>
struct variant_impl<T, Rest...>
{
    static_assert(!std::is_reference_v<T>, "cannot create a variance of references");
    static_assert(!std::is_array_v<T>, "cannot create a variance of array type");

    union data
    {
        T head;
        variant_impl<Rest...> tail;

        data() {}
        ~data() {}
    } _data;

    // NOTE: must be destroyed before
    template <class U, class... Args>
    U& emplace(Args&&... args)
    {
        if constexpr (std::is_same_v<T, U>)
            return *new (cc::placement_new, &_data.head) T(cc::forward<Args>(args)...);
        else
        {
            static_assert(sizeof...(Rest) > 0, "could not find type in variant");
            return _data.tail.template emplace<U>(cc::forward<Args>(args)...);
        }
    }

    template <class U>
    U& get()
    {
        if constexpr (std::is_same_v<T, U>)
            return _data.head;
        else
        {
            static_assert(sizeof...(Rest) > 0, "could not find type in variant");
            return _data.tail.template get<U>();
        }
    }

    template <class U>
    U const& get() const
    {
        if constexpr (std::is_same_v<T, U>)
            return _data.head;
        else
        {
            static_assert(sizeof...(Rest) > 0, "could not find type in variant");
            return _data.tail.template get<U>();
        }
    }

    void destroy(uint8_t idx)
    {
        if (idx == 0)
            _data.head.~T();
        else if constexpr (sizeof...(Rest) > 0)
            _data.tail.destroy(idx - 1);
    }

    template <class F>
    decltype(auto) visit(uint8_t idx, F&& f)
    {
        if constexpr (sizeof...(Rest) > 0)
            if (idx > 0)
                return _data.tail.visit(idx - 1, f);

        return f(_data.head);
    }
    template <class F>
    decltype(auto) visit(uint8_t idx, F&& f) const
    {
        if constexpr (sizeof...(Rest) > 0)
            if (idx > 0)
                return _data.tail.visit(idx - 1, f);

        return f(_data.head);
    }
};
}

// TODO: inherit triviality
// NOTE: currently only supports exact type matches
template <class... Types>
struct variant
{
    static_assert(sizeof...(Types) >= 1, "variant must have at least one type");

    variant()
    {
        using T = detail::first_of<Types...>;
        static_assert(std::is_default_constructible_v<T>, "first type of variant is not default constructible");
        _data.template emplace<T>();
    }

    template <class T, cc::enable_if<!std::is_same_v<T, variant>> = true>
    variant(T&& v) noexcept(noexcept(T(cc::forward<T>(v))))
    {
        constexpr auto i = detail::index_of_type<T, Types...>();
        static_assert(i >= 0, "cannot construct variant from this type. NOTE: implicit conversions are not allowed");
        _idx = uint8_t(i);
        _data.template emplace<std::decay_t<T>>(cc::forward<T>(v));
    }
    variant(variant&& rhs) noexcept
    {
        static_assert((std::is_move_constructible_v<Types> && ...), "all types must be movable");
        // TODO: maybe better comp time if co-recurse
        rhs.visit(
            [&](auto& v)
            {
                using T = std::decay_t<decltype(v)>;
                constexpr auto i = detail::index_of_type<T, Types...>();
                static_assert(i >= 0, "cannot construct variant from this type. NOTE: implicit conversions are not allowed");
                _idx = uint8_t(i);
                _data.template emplace<T>(cc::move(v));
            });
    }
    variant(variant const& rhs)
    {
        static_assert((std::is_copy_constructible_v<Types> && ...), "all types must be copyable");
        // TODO: maybe better comp time if co-recurse
        rhs.visit(
            [&](auto const& v)
            {
                using T = std::decay_t<decltype(v)>;
                constexpr auto i = detail::index_of_type<T, Types...>();
                static_assert(i >= 0, "cannot construct variant from this type. NOTE: implicit conversions are not allowed");
                _idx = uint8_t(i);
                _data.template emplace<T>(v);
            });
    }

    template <class T, cc::enable_if<!std::is_same_v<T, variant>> = true>
    variant& operator=(T&& v) noexcept(noexcept(T(cc::forward<T>(v))))
    {
        constexpr auto i = detail::index_of_type<T, Types...>();
        static_assert(i >= 0, "cannot construct variant from this type. NOTE: implicit conversions are not allowed");
        _data.destroy(_idx);
        _idx = uint8_t(i);
        _data.template emplace<std::decay_t<T>>(cc::forward<T>(v));
        return *this;
    }
    variant& operator=(variant&& rhs) noexcept
    {
        _data.destroy(_idx);
        return rhs.visit(
            [&](auto&& v) -> variant&
            {
                using T = std::decay_t<decltype(v)>;
                this->emplace<T>(cc::move(v));
                return *this;
            });
    }
    variant& operator=(variant const& rhs)
    {
        _data.destroy(_idx);
        return rhs.visit(
            [&](auto const& v) -> variant&
            {
                using T = std::decay_t<decltype(v)>;
                this->emplace<T>(v);
                return *this;
            });
    }

    ~variant() { _data.destroy(_idx); }

    template <class T, class... Args>
    T& emplace(Args&&... args)
    {
        _data.destroy(_idx);
        constexpr auto i = detail::index_of_type<T, Types...>();
        static_assert(i >= 0, "cannot construct variant from this type. NOTE: implicit conversions are not allowed");
        _idx = uint8_t(i);
        return _data.template emplace<T>(cc::forward<Args>(args)...);
    }

    template <class F>
    decltype(auto) visit(F&& f)
    {
        return _data.visit(_idx, f);
    }
    template <class F>
    decltype(auto) visit(F&& f) const
    {
        return _data.visit(_idx, f);
    }

    /// replace this.value with f(this.value)
    /// can change the currently held type
    /// cannot add new types
    template <class F>
    void transform(F&& f)
    {
        _data.visit([&](auto&& v) { *this = f(v); });
    }

    template <class T>
    bool is() const
    {
        return int(_idx) == detail::index_of_type<T, Types...>();
    }

    template <class T>
    T& get()
    {
        CC_ASSERT(this->is<T>());
        static_assert(detail::is_equal_any_of<T, Types...>, "variant does not have this type");
        return _data.template get<T>();
    }
    template <class T>
    T const& get() const
    {
        CC_ASSERT(this->is<T>());
        static_assert(detail::is_equal_any_of<T, Types...>, "variant does not have this type");
        return _data.template get<T>();
    }

private:
    uint8_t _idx = 0;
    detail::variant_impl<Types...> _data;
};

template <class T, class... Types>
bool operator==(T const& lhs, variant<Types...> const& rhs)
{
    return rhs.visit(
        [&lhs](auto const& rhs)
        {
            if constexpr (cc::has_operator_equal<T, decltype(rhs)>)
                return lhs == rhs;
            else
                return false;
        });
}
template <class T, class... Types>
bool operator!=(T const& lhs, variant<Types...> const& rhs)
{
    return rhs.visit(
        [&lhs](auto const& rhs)
        {
            if constexpr (cc::has_operator_not_equal<T, decltype(rhs)>)
                return lhs != rhs;
            else
                return true;
        });
}
template <class T, class... Types>
bool operator==(variant<Types...> const& lhs, T const& rhs)
{
    return lhs.visit(
        [&rhs](auto const& lhs)
        {
            if constexpr (cc::has_operator_equal<decltype(lhs), T>)
                return lhs == rhs;
            else
                return false;
        });
}
template <class T, class... Types>
bool operator!=(variant<Types...> const& lhs, T const& rhs)
{
    return lhs.visit(
        [&rhs](auto const& lhs)
        {
            if constexpr (cc::has_operator_not_equal<decltype(lhs), T>)
                return lhs != rhs;
            else
                return true;
        });
}
}