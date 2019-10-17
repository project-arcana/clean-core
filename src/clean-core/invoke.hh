#pragma once

#include <type_traits>
#include <utility>

#include <clean-core/detail/is_reference_wrapper.hh>

namespace cc
{
// std::invoke implementation without <functional>, from
// https://en.cppreference.com/w/cpp/utility/functional/invoke
namespace detail
{
template <class T, class Type, class T1, class... Args>
auto perform_invoke(Type T::*f, T1&& t1, Args&&... args)
{
    if constexpr (std::is_member_function_pointer_v<decltype(f)>)
    {
        if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
            return (std::forward<T1>(t1).*f)(std::forward<Args>(args)...);
        else if constexpr (cc::is_reference_wrapper<std::decay_t<T1>>)
            return (t1.get().*f)(std::forward<Args>(args)...);
        else
            return ((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...);
    }
    else
    {
        static_assert(std::is_member_object_pointer_v<decltype(f)>);
        static_assert(sizeof...(args) == 0);
        if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
            return std::forward<T1>(t1).*f;
        else if constexpr (cc::is_reference_wrapper<std::decay_t<T1>>)
            return t1.get().*f;
        else
            return (*std::forward<T1>(t1)).*f;
    }
}

template <class F, class... Args>
auto perform_invoke(F&& f, Args&&... args)
{
    return std::forward<F>(f)(std::forward<Args>(args)...);
}
}

template <class F, class... Args>
std::invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
{
    return detail::perform_invoke(std::forward<F>(f), std::forward<Args>(args)...);
}
}
