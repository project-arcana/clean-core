#pragma once

#include <cstdint>

#include <clean-core/fwd.hh>
#include <clean-core/hash_combine.hh>
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

    template <class C, class D>
    constexpr bool operator==(pair<C, D> const& rhs) const
    {
        return first == rhs.first && second == rhs.second;
    }
    template <class C, class D>
    constexpr bool operator!=(pair<C, D> const& rhs) const
    {
        return first != rhs.first || second != rhs.second;
    }
};

template <class A, class B>
struct hash<pair<A, B>>
{
    [[nodiscard]] constexpr uint64_t operator()(pair<A, B> const& v) const noexcept
    {
        return cc::hash_combine(hash<A>{}(v.first), hash<B>{}(v.second));
    }
};

template <class I, class A, class B>
constexpr void introspect(I&& i, pair<A, B>& p)
{
    i(p.first, "first");
    i(p.second, "second");
}
}
