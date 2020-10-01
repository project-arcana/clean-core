#include <clean-core/assert.hh>

#include <clean-core/breakpoint.hh>

#include <cstdio>
#include <cstdlib>

namespace
{
struct assertion_not_handled
{
};

using assertion_handler_t = void (*)(cc::detail::assertion_info const&);

thread_local assertion_handler_t s_current_handler = nullptr;

void default_assertion_handler(cc::detail::assertion_info const& info)
{
    fflush(stdout);

    fprintf(stderr, "assertion `%s' failed.\n", info.expr);

    if (info.msg != nullptr)
    {
        fprintf(stderr, "  ---\n%s\n  ---\n", info.msg);
    }

    fprintf(stderr, "  in %s\n", info.func);
    fprintf(stderr, "  file %s:%d\n", info.file, info.line);
    fflush(stderr);

    // TODO: stacktrace

#if !defined(CC_RELEASE) && !defined(CC_OS_WINDOWS)
    // on win32, this suppresses the CRT abort/retry debugger attach message
    cc::breakpoint();
#endif

    std::abort();
}
} // namespace

void cc::detail::assertion_failed(assertion_info const& info)
{
    if (s_current_handler)
        s_current_handler(info);
    else
        default_assertion_handler(info);

    throw assertion_not_handled{};
}

void cc::set_assertion_handler(void (*handler)(detail::assertion_info const&)) { s_current_handler = handler; }
