#pragma once

#include <clean-core/fwd.hh>
#include <clean-core/move.hh>

namespace cc
{
template <class A, class B>
struct pair
{
    A first;
    B second;

    pair() = default;
    pair(A f, B s) : first(cc::move(f)), second(cc::move(s)) {}
};
}
