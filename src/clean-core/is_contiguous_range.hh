#pragma once

#include <cstddef>
#include <type_traits>

namespace cc
{
namespace detail
{
template <class Container, class ElementT>
auto contiguous_range_test(Container* c) -> decltype(static_cast<ElementT*>(c->data()), static_cast<decltype(sizeof(0))>(c->size()), 0);
template <class Container, class ElementT>
char contiguous_range_test(...);

template <class Container>
auto contiguous_range_test_of_pods(Container* c)
    -> std::enable_if_t<std::is_trivially_copyable_v<std::decay_t<decltype(*c->data())>>, // c->data exists and is pointer-to-pod
                        decltype(static_cast<decltype(sizeof(0))>(c->size()), 0)>;        // c->size exists and is integral
template <class Container>
char contiguous_range_test_of_pods(...);
}

template <class Container, class ElementT>
static constexpr bool is_contiguous_range = sizeof(detail::contiguous_range_test<std::remove_reference_t<Container>, ElementT>(nullptr)) == sizeof(int);

template <class ElementT, size_t N>
static constexpr bool is_contiguous_range<ElementT (&)[N], ElementT> = true;

template <class ElementT, size_t N>
static constexpr bool is_contiguous_range<ElementT (&)[N], void const> = true;

template <class Container>
static constexpr bool is_any_contiguous_range = is_contiguous_range<Container, void const>;


template <class Container>
static constexpr bool is_any_contiguous_range_of_pods
    = sizeof(detail::contiguous_range_test_of_pods<std::remove_reference_t<Container>>(nullptr)) == sizeof(int);
template <class ElementT, size_t N>
static constexpr bool is_any_contiguous_range_of_pods<ElementT (&)[N]> = std::is_trivially_copyable_v<ElementT>;
}
