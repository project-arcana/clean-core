#pragma once

#include <type_traits>
#include <utility>

namespace cc
{
namespace detail
{
template <class Container, class ElementT>
auto container_test(Container* c) -> decltype(static_cast<ElementT*>(c->data()), static_cast<decltype(sizeof(0))>(c->size()), 0);
template <class Container, class ElementT>
char container_test(...);
}

template <class Container, class ElementT>
static constexpr bool is_container = sizeof(detail::container_test<Container, ElementT>(nullptr)) == sizeof(int);
}
