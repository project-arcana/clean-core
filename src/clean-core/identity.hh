#pragma once

#include <clean-core/forward.hh>

namespace cc
{
/// general-purpose identity function object
/// (preserves value category)
struct identity
{
    template <class T>
    constexpr T&& operator()(T&& v) const noexcept
    {
        return cc::forward<T>(v);
    }
};
}
