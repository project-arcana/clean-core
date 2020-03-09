#pragma once

#include <type_traits>

namespace cc
{
namespace detail
{
template <class Container, class ElementT, class = void>
struct is_range_t : std::false_type
{
};
template <class Container, class ElementT>
struct is_range_t<Container,
                  ElementT,
                  std::void_t<                                                              //
                      decltype(static_cast<ElementT&>(*std::declval<Container>().begin())), //
                      decltype(std::declval<Container>().end())                             //
                      >> : std::true_type
{
};

template <class Container, class = void>
struct is_any_range_t : std::false_type
{
};
template <class Container>
struct is_any_range_t<Container,
                      std::void_t<                                     //
                          decltype(std::declval<Container>().begin()), //
                          decltype(std::declval<Container>().end())    //
                          >> : std::true_type
{
};
}

template <class Container, class ElementT>
static constexpr bool is_range = detail::is_range_t<Container, ElementT>::value;
template <class Container>
static constexpr bool is_any_range = detail::is_any_range_t<Container>::value;
}
