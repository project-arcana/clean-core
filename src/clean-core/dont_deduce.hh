#pragma once

namespace cc
{
namespace detail
{
template <class T>
struct dont_deduce
{
    using type = T;
};
}

/**
 * helper typedef for disabling template argument deduction
 * see https://artificial-mind.net/blog/2020/09/26/dont-deduce
 *
 * example:
 *
 *   template <class T>
 *   vec3<T> operator*(vec3<T> const& a, dont_deduce<T> b);
 *
 *   // otherwise this won't work:
 *   vec3<foat> v = ...;
 *   v = v * 3;
 */
template <class T>
using dont_deduce = typename detail::dont_deduce<T>::type;
}
