#pragma once

#include <cc/macros>

// least overhead assertion macros
// see https://godbolt.org/z/mWdaj3
// [[unlikely]] produces more code in O0 so it is only used outside of debug
// decltype(...) is an unevaluated context, thus eliminating any potential side effect
// assertion handler is customizable

// ASSERT(cond) aborts if `cond` is false
// BOUND_CHECK(var, lb, ub) asserts `lb <= var && var < ub`
// NOTE: neither macro must contain side effects!

// compile flags
// CC_ENABLE_ASSERTIONS enables assertions
// CC_ENABLE_BOUND_CHECKING enables bound checking

#if !defined(CC_ENABLE_ASSERTIONS)
#define ASSERT(condition)                              \
    do                                                 \
    {                                                  \
        [[maybe_unused]] decltype((condition), int) b; \
    } while (0) // force ;

#elif defined(CC_DEBUG)
#define ASSERT(condition)                                                                                                                           \
    do                                                                                                                                              \
    {                                                                                                                                               \
        static constexpr ::cc::detail::assertion_info CC_MACRO_JOIN(_cc_assert_info_, __LINE__) = {#condition, CC_PRETTY_FUNC, __FILE__, __LINE__}; \
        if (!(condition))                                                                                                                           \
            ::cc::detail::assertion_failed(CC_MACRO_JOIN(_cc_assert_info_, __LINE__));                                                              \
    } while (0) // force ;

#else
#define ASSERT(condition)                                                                                                                           \
    do                                                                                                                                              \
    {                                                                                                                                               \
        static constexpr ::cc::detail::assertion_info CC_MACRO_JOIN(_cc_assert_info_, __LINE__) = {#condition, CC_PRETTY_FUNC, __FILE__, __LINE__}; \
        if (CC_UNLIKELY(!(condition)))                                                                                                              \
            ::cc::detail::assertion_failed(CC_MACRO_JOIN(_cc_assert_info_, __LINE__));                                                              \
    } while (0) // force ;
#endif


#ifdef CC_ENABLE_BOUND_CHECKING
#define CC_ASSERT_IN_BOUNDS(var, lb, ub) ASSERT((lb) <= (var) && (var) < (ub) && "bound check")
#else
#define CC_ASSERT_IN_BOUNDS(var, lb, ub) CC_UNUSED((lb) <= (var) && (var) < (ub))
#endif

#ifdef CC_ENABLE_NULL_CHECKING
#define CC_ASSERT_IS_NULL(p) ASSERT((p) == nullptr && "must be null");
#define CC_ASSERT_NOT_NULL(p) ASSERT((p) != nullptr && "must not be null");
#else
#define CC_ASSERT_IS_NULL(p) CC_UNUSED((p) == nullptr && "");
#define CC_ASSERT_NOT_NULL(p) CC_UNUSED((p) != nullptr && "");
#endif

#ifdef CC_ENABLE_CONTRACT_CHECKING
#define CC_ASSERT_CONTRACT(condition) ASSERT(condition && "contract violation")
#else
#define CC_ASSERT_CONTRACT(condition) CC_UNUSED(condition && "")
#endif

namespace cc::detail
{
struct assertion_info
{
    char const* expr;
    char const* func;
    char const* file;
    int line;
};

CC_COLD_FUNC CC_DONT_INLINE void assertion_failed(assertion_info const& info);
} // namespace cc::detail

namespace cc
{
/// handler that is called whenever an assertion is violated
/// pass nullptr to reset to default handler
/// this is a thread_local handler
/// the handler must be replaced before it is deleted (non-owning view)
void set_assertion_handler(void (*handler)(detail::assertion_info const& info));
} // namespace cc
