#pragma once

#include <type_traits>
#include <utility> // index seq and tuple protocol

#include <clean-core/forward.hh>
#include <clean-core/get.hh>
#include <clean-core/invoke.hh>

namespace cc
{
namespace detail
{
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return cc::invoke(cc::forward<F>(f), cc::get<I>(cc::forward<Tuple>(t))...);
}
}

template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
    return detail::apply_impl(cc::forward<F>(f), cc::forward<Tuple>(t), std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}
}
