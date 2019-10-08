#pragma once

#include <clean-core/always_false.hh>
#include <clean-core/fwd.hh>
#include <clean-core/typedefs.hh>

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
