#pragma once

#include <utility> // for tuple_size

#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/always_false.hh>
#include <clean-core/hash_combine.hh>

namespace cc
{
namespace detail
{
template <class... Ts>
struct tuple_impl;
template <>
struct tuple_impl<>
{
    template <size_t I>
    constexpr void get() const
    {
        static_assert(cc::always_false<I>, "cannot get element of empty tuple");
    }
};
template <class T>
struct tuple_impl<T>
{
    T value;

    constexpr tuple_impl() = default;
    constexpr tuple_impl(T v) : value(cc::forward<T>(v)) {}

    template <size_t I>
    [[nodiscard]] constexpr T& get()
    {
        static_assert(I == 0, "invalid tuple index");
        return value;
    }
    template <size_t I>
    [[nodiscard]] constexpr T const& get() const
    {
        static_assert(I == 0, "invalid tuple index");
        return value;
    }
};
template <class T, class U, class... Ts>
struct tuple_impl<T, U, Ts...> : tuple_impl<U, Ts...>
{
    T value;

    constexpr tuple_impl() = default;
    constexpr tuple_impl(T v, U u, Ts... ts) : tuple_impl<U, Ts...>(cc::forward<U>(u), cc::forward<Ts>(ts)...), value(cc::forward<T>(v)) {}

    template <size_t I>
    [[nodiscard]] constexpr decltype(auto) get()
    {
        if constexpr (I == 0)
            return (value);
        else
            return tuple_impl<U, Ts...>::template get<I - 1>();
    }
    template <size_t I>
    [[nodiscard]] constexpr decltype(auto) get() const
    {
        if constexpr (I == 0)
            return (value);
        else
            return tuple_impl<U, Ts...>::template get<I - 1>();
    }
};

template <class... Types>
struct tuple_helper
{
    template <class... OtherTypes>
    struct other_tuple
    {
        template <size_t... I>
        static constexpr bool are_equal(tuple<Types...> const& lhs, tuple<OtherTypes...> const& rhs, std::index_sequence<I...>)
        {
            static_assert(sizeof...(Types) == sizeof...(OtherTypes), "wrong number of elements");
            return (true && ... && (lhs.template get<I>() == rhs.template get<I>()));
        }
        template <size_t... I>
        static constexpr bool are_not_equal(tuple<Types...> const& lhs, tuple<OtherTypes...> const& rhs, std::index_sequence<I...>)
        {
            static_assert(sizeof...(Types) == sizeof...(OtherTypes), "wrong number of elements");
            return (false || ... || (lhs.template get<I>() != rhs.template get<I>()));
        }
    };

    template <size_t... I>
    static constexpr hash_t make_hash(tuple<Types...> const& v, std::index_sequence<I...>)
    {
        return cc::hash_combine(cc::hash<Types>{}(v.template get<I>())...);
    }
};
}

/// a light-weight tuple type
/// mainly used for metaprogramming, thus not particularly user-friendly
template <class... Types>
struct tuple : private detail::tuple_impl<Types...>
{
    using detail::tuple_impl<Types...>::get;
    using detail::tuple_impl<Types...>::tuple_impl;

    template <class... OtherTypes>
    constexpr bool operator==(tuple<OtherTypes...> const& rhs) const
    {
        return detail::tuple_helper<Types...>::template other_tuple<OtherTypes...>::template are_equal(*this, rhs, std::index_sequence_for<Types...>{});
    }
    template <class... OtherTypes>
    constexpr bool operator!=(tuple<OtherTypes...> const& rhs) const
    {
        return detail::tuple_helper<Types...>::template other_tuple<OtherTypes...>::template are_not_equal(*this, rhs, std::index_sequence_for<Types...>{});
    }
};

// deduction guide
template <class... Types>
tuple(Types...)->tuple<Types...>;

// hash
template <class... Ts>
struct hash<tuple<Ts...>>
{
    [[nodiscard]] constexpr hash_t operator()(tuple<Ts...> const& v) const noexcept
    {
        return cc::detail::tuple_helper<Ts...>::make_hash(v, std::index_sequence_for<Ts...>{});
    }
};
}

namespace std
{
template <class... Types>
struct tuple_size<cc::tuple<Types...>> : public std::integral_constant<std::size_t, sizeof...(Types)>
{
};
template <std::size_t I, class T, class... Rest>
struct tuple_element<I, cc::tuple<T, Rest...>> : tuple_element<I - 1, cc::tuple<Rest...>>
{
};
template <class T, class... Rest>
struct tuple_element<0, cc::tuple<T, Rest...>>
{
    using type = T;
};
}
