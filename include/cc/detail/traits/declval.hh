#pragma once

#include <cc/detail/traits/add_reference.hh>

namespace cc
{
// must be a function because it's an lvalue otherwise
template <class T>
static constexpr add_rvalue_reference<T> declval() noexcept;

template <class T>
static inline T declvar;
} // namespace cc
