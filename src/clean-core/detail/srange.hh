#pragma once

#include <clean-core/forward.hh>
#include <clean-core/sentinel.hh>

namespace cc::detail
{
// generic sentinel range
// (requires ItT != cc::sentinel check and copyable iterator)
template <class ItT>
struct srange
{
    template <class... Args>
    explicit srange(Args&&... args) : it(cc::forward<Args>(args)...)
    {
    }

    auto begin() { return it; }
    auto begin() const { return it; }
    cc::sentinel end() const { return {}; }

private:
    ItT it;
};
}
