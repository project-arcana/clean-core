#pragma once

#include <clean-core/fwd_box.hh>

namespace cc
{
template <class T>
using pimpl = fwd_box<T>;

template <class T, class... Args>
pimpl<T> make_pimpl(Args&&... args)
{
    return cc::make_fwd_box<T>(cc::forward<Args>(args)...);
}
}
