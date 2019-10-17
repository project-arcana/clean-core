#pragma once

#include <type_traits>
#include <utility>

namespace cc
{
namespace detail
{
template <class T>
struct test_reference_wrappper : std::false_type
{
};
template <class U>
struct test_reference_wrappper<std::reference_wrapper<U>> : std::true_type
{
};
} // namespace detail

template <class T>
constexpr bool is_reference_wrapper = detail::test_reference_wrappper<T>::value;
}
