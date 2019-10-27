#pragma once

#include <clean-core/forward.hh>
#include <clean-core/macros.hh>

/**
 * execute code at scope-exit:
 *   CC_DEFER {
 *      ...
 *   };
 */
#define CC_DEFER auto const CC_MACRO_JOIN(_cc_deferred_, __COUNTER__) = ::cc::detail::def_tag{} + [&]

namespace cc::detail
{
template <class F>
struct deferred
{
    F f;

    ~deferred() { f(); }
};

struct def_tag
{
};

template <class F>
deferred<F> operator+(def_tag, F&& f)
{
    return {cc::forward<F>(f)};
}
}
