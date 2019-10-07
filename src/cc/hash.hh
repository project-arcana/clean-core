#pragma once

#include <cc/always_false.hh>
#include <cc/fwd.hh>
#include <cc/typedefs.hh>

namespace cc
{
template <class T, class>
struct hash
{
    static_assert(always_false<T>, "no hash<> specialization found");

    // hash_t operator()(T const& value) const noexcept { ... }
};

template <class T>
struct hash<T*>
{
    [[nodiscard]] hash_t operator()(T* const& value) const noexcept { return hash_t(value); }
};

}
