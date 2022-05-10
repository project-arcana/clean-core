#pragma once

#include <clean-core/macros.hh>

// CC_ASSERT(cond) aborts if `cond` is false
// CC_BOUND_CHECK(var, lb, ub) asserts `lb <= var && var < ub`
// NOTE: neither macro must contain side effects!

// compile flags
// CC_ENABLE_ASSERTIONS enables assertions
// CC_ENABLE_BOUND_CHECKING enables bound checking


// the debugger should break right in the assert macro, so this cannot hide in a function call

#ifdef CC_COMPILER_MSVC
// __debugbreak() terminates immediately without an attached debugger
#if _MSC_VER >= 1400
#define CC_DEBUG_BREAK() (::cc::detail::is_debugger_connected() ? __debugbreak() : void(0))
#else
#define CC_DEBUG_BREAK() (::cc::detail::is_debugger_connected() ? _asm int 0x03 : void(0))
#endif
#elif defined(CC_COMPILER_POSIX)
// __builtin_trap() causes an illegal instruction and crashes without an attached debugger
#define CC_DEBUG_BREAK() (::cc::detail::is_debugger_connected() ? __builtin_trap() : void(0))
#else
#define CC_DEBUG_BREAK() void(0)
#endif

#define CC_BREAK_AND_ABORT() (CC_DEBUG_BREAK(), ::cc::detail::perform_abort())

// least overhead assertion macros
// see https://godbolt.org/z/aWW1f8
// assertion handler is customizable

#define CC_DETAIL_EXECUTE_ASSERT(condition, msg)                                                                                                    \
    (CC_UNLIKELY(!(condition))                                                                                                                      \
         ? (::cc::detail::assertion_failed({#condition, CC_PRETTY_FUNC, __FILE__, msg, __LINE__}), CC_DEBUG_BREAK(), ::cc::detail::perform_abort()) \
         : void(0)) // force ;

#define CC_RUNTIME_ASSERT(condition) CC_DETAIL_EXECUTE_ASSERT(condition, nullptr)
#define CC_RUNTIME_ASSERT_MSG(condition, msg) CC_DETAIL_EXECUTE_ASSERT(condition, msg)

#if !defined(CC_ENABLE_ASSERTIONS)
#define CC_ASSERT(condition) CC_UNUSED(condition)
#define CC_ASSERT_MSG(condition, msg) CC_UNUSED(condition)
#else
#define CC_ASSERT(condition) CC_DETAIL_EXECUTE_ASSERT(condition, nullptr)
#define CC_ASSERT_MSG(condition, msg) CC_DETAIL_EXECUTE_ASSERT(condition, msg)
#endif

#ifdef CC_ENABLE_BOUND_CHECKING
#define CC_ASSERT_IN_BOUNDS(var, lb, ub) CC_ASSERT((lb) <= (var) && (var) < (ub) && "bound check")
#else
#define CC_ASSERT_IN_BOUNDS(var, lb, ub) CC_UNUSED((lb) <= (var) && (var) < (ub))
#endif

#ifdef CC_ENABLE_NULL_CHECKING
#define CC_ASSERT_IS_NULL(p) CC_ASSERT((p) == nullptr && "must be null")
#define CC_ASSERT_NOT_NULL(p) CC_ASSERT((p) != nullptr && "must not be null")
#else
#define CC_ASSERT_IS_NULL(p) CC_UNUSED((p) == nullptr && "")
#define CC_ASSERT_NOT_NULL(p) CC_UNUSED((p) != nullptr && "")
#endif

#ifdef CC_ENABLE_CONTRACT_CHECKING
#define CC_CONTRACT(condition) CC_ASSERT((condition) && "contract violation")
#else
#define CC_CONTRACT(condition) CC_UNUSED((condition) && "")
#endif

#ifdef CC_ENABLE_ASSERTIONS
#define CC_UNREACHABLE(msg)                                                                                                             \
    (::cc::detail::assertion_failed({"unreachable code reached: " msg, CC_PRETTY_FUNC, __FILE__, nullptr, __LINE__}), CC_DEBUG_BREAK(), \
     ::cc::detail::perform_abort(), CC_BUILTIN_UNREACHABLE)
#else
#define CC_UNREACHABLE(msg) CC_BUILTIN_UNREACHABLE
#endif

// workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
#if defined(CC_COMPILER_GCC) && __GNUC__ < 9
#define CC_UNREACHABLE_SWITCH_WORKAROUND(type)                \
    if (type != decltype(type){} || type == decltype(type){}) \
        CC_UNREACHABLE("unhandled case for " #type);          \
    else                                                      \
        return {} // force ;
#else
#define CC_UNREACHABLE_SWITCH_WORKAROUND(type) CC_UNREACHABLE("unhandled case for " #type)
#endif

namespace cc::detail
{
struct assertion_info
{
    char const* expr;
    char const* func;
    char const* file;
    char const* msg;
    int line;
};

CC_COLD_FUNC CC_DONT_INLINE void assertion_failed(assertion_info const& info);

CC_COLD_FUNC CC_DONT_INLINE bool is_debugger_connected();

/// calls std::abort(), avoids includes
CC_COLD_FUNC CC_DONT_INLINE void perform_abort();
}

namespace cc
{
/// handler that is called whenever an assertion is violated
/// pass nullptr to reset to default handler
/// this is a thread_local handler
/// the handler must be replaced before it is deleted (non-owning view)
void set_assertion_handler(void (*handler)(detail::assertion_info const& info));
}
