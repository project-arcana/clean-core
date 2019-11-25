#pragma once

#include <type_traits>

namespace cc
{
namespace detail
{
template <class Container, class ElementT>
auto range_test(Container* c) -> decltype(static_cast<ElementT&>(*c->begin()), (void)c->end(), 0);
template <class Container, class ElementT>
char range_test(...);

template <class Container>
auto any_range_test(Container* c) -> decltype((void)(*c->begin()), (void)c->end(), 0);
template <class Container>
char any_range_test(...);
}

template <class Container, class ElementT>
static constexpr bool is_range = sizeof(detail::range_test<Container, ElementT>(nullptr)) == sizeof(int);
template <class Container>
static constexpr bool is_any_range = sizeof(detail::any_range_test<Container>(nullptr)) == sizeof(int);
}
