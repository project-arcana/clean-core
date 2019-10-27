#pragma once

#include <clean-core/detail/remove_reference.hh>

namespace cc
{
template <class T>
constexpr remove_reference<T>&& move(T&& t) noexcept
{
    return static_cast<remove_reference<T>&&>(t);
}
}
