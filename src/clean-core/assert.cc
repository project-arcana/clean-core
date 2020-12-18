#include <clean-core/assert.hh>

#include <clean-core/breakpoint.hh>
#include <clean-core/macros.hh>

#ifdef CC_OS_WINDOWS
#include <clean-core/detail/lib/StackWalker.hh>
#endif

#include <cstdio>
#include <cstdlib>

namespace
{
#ifdef CC_OS_WINDOWS

class CallstackOutputWalker final : public StackWalker
{
protected:
    void OnOutput(LPCSTR) override {}
    void OnCallstackEntry(CallstackEntryType eType, CallstackEntry& entry) override
    {
        if (eType == CallstackEntryType::firstEntry)
            return; // the first entry is StackWalker::ShowCallstack(), skip

        if (entry.lineFileName[0]) // callstack inside of known source files
        {
            // this file/line formatting allows clickable "links" in VC output, it's not arbitrary
            fprintf(stderr, "  %s (%lu) : %s\n", entry.lineFileName, entry.lineNumber, entry.name);
        }
        else // callstack in external modules (DLLs, Kernel)
        {
            fprintf(stderr, "  %s : %s\n", entry.loadedImageName, entry.name);
        }
    }
};

// macro instead of function to reduce noise in the callstack
#define CC_PRINT_STACK_TRACE()               \
    do                                       \
    {                                        \
        fprintf(stderr, "\nstack trace:\n"); \
        CallstackOutputWalker sw;            \
        sw.ShowCallstack();                  \
    } while (0)

#else

// TODO
#define CC_PRINT_STACK_TRACE()

#endif

struct assertion_not_handled
{
};

using assertion_handler_t = void (*)(cc::detail::assertion_info const&);

thread_local assertion_handler_t s_current_handler = nullptr;

void default_assertion_handler(cc::detail::assertion_info const& info)
{
    fflush(stdout);

    fprintf(stderr, "\nassertion `%s' failed.\n", info.expr);

    if (info.msg != nullptr)
    {
        fprintf(stderr, "  ---\n%s\n  ---\n", info.msg);
    }

    fprintf(stderr, "  in %s\n", info.func);
    fprintf(stderr, "  file %s:%d\n", info.file, info.line);
    fflush(stderr);

    CC_PRINT_STACK_TRACE();
    fflush(stderr);

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
