#pragma once

namespace cc
{
namespace detail
{
template <class T>
auto try_add_lvalue_reference(int) -> T&;
template <class T>
auto try_add_lvalue_reference(...) -> T;

template <class T>
auto try_add_rvalue_reference(int) -> T&&;
template <class T>
auto try_add_rvalue_reference(...) -> T;
} // namespace detail

template <class T>
using add_lvalue_reference = decltype(detail::try_add_lvalue_reference<T>(0));

template <class T>
using add_rvalue_reference = decltype(detail::try_add_rvalue_reference<T>(0));
} // namespace cc
