#pragma once

#include <type_traits>

#include <clean-core/detail/is_reference_wrapper.hh>
#include <clean-core/forward.hh>
#include <clean-core/macros.hh>

namespace cc
{
// std::invoke implementation without <functional>, from
// https://en.cppreference.com/w/cpp/utility/functional/invoke
namespace detail
{
template <class T, class Type, class T1, class... Args>
CC_FORCE_INLINE constexpr decltype(auto) perform_invoke(Type T::*f, T1&& t1, Args&&... args)
{
    if constexpr (std::is_member_function_pointer_v<decltype(f)>)
    {
        if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
            return (cc::forward<T1>(t1).*f)(cc::forward<Args>(args)...);
        else if constexpr (cc::is_reference_wrapper<std::decay_t<T1>>)
            return (t1.get().*f)(cc::forward<Args>(args)...);
        else
            return ((*cc::forward<T1>(t1)).*f)(cc::forward<Args>(args)...);
    }
    else
    {
        static_assert(std::is_member_object_pointer_v<decltype(f)>);
        static_assert(sizeof...(args) == 0);
        if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
            return cc::forward<T1>(t1).*f;
        else if constexpr (cc::is_reference_wrapper<std::decay_t<T1>>)
            return t1.get().*f;
        else
            return (*cc::forward<T1>(t1)).*f;
    }
}

template <class F, class... Args>
CC_FORCE_INLINE constexpr decltype(auto) perform_invoke(F&& f, Args&&... args)
{
    return cc::forward<F>(f)(cc::forward<Args>(args)...);
}
}

template <class F, class... Args>
CC_FORCE_INLINE constexpr decltype(auto) invoke(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
{
    return detail::perform_invoke(cc::forward<F>(f), cc::forward<Args>(args)...);
}
}
