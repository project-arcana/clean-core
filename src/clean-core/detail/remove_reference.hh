#pragma once

namespace cc
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
template <class T>
using remove_reference = typename remove_reference_t<T>::type;
}
