#pragma once

#include <type_traits>

namespace cc
{
namespace detail
{
template <class Container, class ElementT>
auto contiguous_range_test(Container* c) -> decltype(static_cast<ElementT*>(c->data()), static_cast<decltype(sizeof(0))>(c->size()), 0);
template <class Container, class ElementT>
char contiguous_range_test(...);
}

template <class Container, class ElementT>
static constexpr bool is_contiguous_range = sizeof(detail::contiguous_range_test<std::remove_reference_t<Container>, ElementT>(nullptr)) == sizeof(int);

template <class ElementT, size_t N>
static constexpr bool is_contiguous_range<ElementT (&)[N], ElementT> = true;

template <class Container>
static constexpr bool is_any_contiguous_range = is_contiguous_range<Container, void const>;
}
