#pragma once

namespace cc
{
namespace detail
{
template <class T>
struct remove_reference_t
{
    using type = T;
};
template <class T>
struct remove_reference_t<T&>
{
    using type = T;
};
template <class T>
struct remove_reference_t<T&&>
{
    using type = T;
};
} // namespace detail

template <class T>
using remove_reference = typename detail::remove_reference_t<T>::type;
} // namespace cc
