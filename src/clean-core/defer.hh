#pragma once

#include <clean-core/forward.hh>
#include <clean-core/macros.hh>

/**
 * execute code at scope-exit:
 *
 *   begin();
 *   CC_DEFER { end(); };
 *
 */
#define CC_DEFER auto const CC_MACRO_JOIN(_cc_deferred_, __COUNTER__) = ::cc::detail::deferred_tag{} + [&]

/**
 * execute code at scope-exit in calling function:
 *
 *   auto scoped_foo() {
 *     begin();
 *     CC_RETURN_DEFER { this->end(); };
 *   }
 *
 */
#define CC_RETURN_DEFER return ::cc::detail::deferred_tag{} + [=]

namespace cc::detail
{
template <class F>
struct deferred
{
    F f;

    deferred(deferred const&) = delete;
    deferred(deferred&&) = delete;

    ~deferred() { f(); }
};

struct deferred_tag
{
};

template <class F>
deferred<F> operator+(deferred_tag, F&& f)
{
    return {cc::forward<F>(f)};
}
}
