#pragma once

#include <clean-core/detail/reference_wrapper.hh>
#include <clean-core/invoke.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
namespace detail
{
template <class T>
struct id
{
    using type = T;
};

template <size_t... N>
struct sizes : id<sizes<N...>>
{
};

template <size_t N, class... T>
struct choose_impl;

template <size_t N, class H, class... Ts>
struct choose_impl<N, H, Ts...> : choose_impl<N - 1, Ts...>
{
};

template <class H, class... T>
struct choose_impl<0, H, T...> : id<H>
{
};

template <size_t N, class... Ts>
using choose_t = typename choose_impl<N, Ts...>::type;

template <size_t L, size_t I = 0, class S = sizes<>>
struct range_impl;

template <size_t L, size_t I, size_t... N>
struct range_impl<L, I, sizes<N...>> : range_impl<L, I + 1, sizes<N..., I>>
{
};

template <size_t L, size_t... N>
struct range_impl<L, L, sizes<N...>> : sizes<N...>
{
};

template <size_t L>
using range_t = typename range_impl<L>::type;

template <size_t N, class T>
class tuple_element_impl
{
    T elem;

public:
    T& get() { return elem; }
    const T& get() const { return elem; }
};

template <class N, class... T>
class tuple_impl;

template <size_t... N, class... T>
class tuple_impl<sizes<N...>, T...> : tuple_element_impl<N, T>...
{
    template <size_t M>
    using pick = choose_t<M, T...>;
    template <size_t M>
    using elem = tuple_element_impl<M, pick<M>>;

public:
    template <size_t M>
    pick<M>& get()
    {
        return elem<M>::get();
    }

    template <size_t M>
    const pick<M>& get() const
    {
        return elem<M>::get();
    }
};

template <class F, class Tuple, std::size_t... I>
constexpr auto apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return cc::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}
}

// std::tuple basic implementation, adapted from http://coliru.stacked-crooked.com/a/ac1e7947152acd6a
template <class... Ts>
struct tuple : detail::tuple_impl<detail::range_t<sizeof...(Ts)>, Ts...>
{
    static constexpr size_t size() { return sizeof...(Ts); }
};

// std::make_tuple and std::apply implementations, from cppreference.com
template <class... Ts>
constexpr auto make_tuple(Ts&&... args)
{
    return cc::tuple<cc::reference_decay_t<Ts>...>(std::forward<Ts>(args)...);
}

template <class F, class Tuple>
constexpr auto apply(F&& f, Tuple&& t)
{
    return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}
}
