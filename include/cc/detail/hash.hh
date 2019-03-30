#pragma once

#include <cc/detail/error.hh>
#include <cc/fwd/hash.hh>
#include <cc/typedefs>

namespace cc
{
template <class T>
struct hash
{
    static_assert(detail::error<T>::value, "no hash specialization found");

    // hash_t operator()(T const& value) const noexcept { ... }
};

template <class T>
struct hash<T*>
{
    hash_t operator()(T* const& value) const noexcept { return hash_t(value); }
};

} // namespace cc
