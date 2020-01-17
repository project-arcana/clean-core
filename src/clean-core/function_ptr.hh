#pragma once

#include <clean-core/always_false.hh>

namespace cc
{
namespace detail
{
template <class T>
struct function_ptr_t
{
    static_assert(always_false<T>, "this should only be used with function signatures");
};
template <class R, class... Args>
struct function_ptr_t<R(Args...)>
{
    using type = R (*)(Args...);
};
template <class R, class... Args>
struct function_ptr_t<R(Args...) noexcept>
{
    using type = R (*)(Args...) noexcept;
};
}
template <class T>
using function_ptr = typename detail::function_ptr_t<T>::type;
}
