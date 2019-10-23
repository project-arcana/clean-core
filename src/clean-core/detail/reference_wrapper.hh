#pragma once

#include <type_traits>
#include <utility>

namespace cc
{
namespace detail
{
template <class T>
struct test_reference_wrapper : std::false_type
{
};
template <class U>
struct test_reference_wrapper<std::reference_wrapper<U>> : std::true_type
{
};

template <class T>
struct unwrap_reference_wrapper
{
    using type = T;
};

template <class T>
struct unwrap_reference_wrapper<std::reference_wrapper<T>>
{
    using type = T&;
};

} // namespace detail

template <class T>
constexpr bool is_reference_wrapper = detail::test_reference_wrapper<T>::value;

template <class T>
using reference_decay_t = typename detail::unwrap_reference_wrapper<typename std::decay<T>::type>::type;
}
