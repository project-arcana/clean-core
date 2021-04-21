#include <clean-core/assert.hh>

#include <csignal>

#include <clean-core/macros.hh>

#ifdef CC_OS_WINDOWS
// clang-format off
#include <clean-core/native/detail/win32_sanitize_before.inl>
#include <Windows.h> // required for stackwalker
#include <crtdbg.h> // _CrtDebugReport

#include <clean-core/detail/lib/StackWalker.hh>

#include <clean-core/native/detail/win32_sanitize_after.inl>
// clang-format on

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

#if defined(CC_OS_WINDOWS) && defined(_DEBUG)
    // in debug and without an attached debugger, give the user a chance to attach after this assert was hit
    if (!IsDebuggerPresent())
    {
        // Classic "Press Retry to Debug" CRT message
        _CrtDbgReport(_CRT_ASSERT, info.file, info.line, nullptr, "%s", info.expr);
        // we only survive this call if Retry or Ignore was clicked
        // let control run into the __debugbreak next
    }
#endif // CC_OS_WINDOWS && _DEBUG
}
} // namespace

void cc::detail::assertion_failed(assertion_info const& info)
{
    if (s_current_handler)
    {
        s_current_handler(info);
    }
    else
    {
        default_assertion_handler(info);
    }
}

bool cc::detail::is_debugger_connected()
{
#ifdef CC_OS_WINDOWS
    return IsDebuggerPresent();
#else
    // unimplemented
    return true;
#endif
}

void cc::detail::perform_abort()
{
#ifdef CC_OS_WINDOWS
    // throw a SEH exception instead of calling std::abort() for telemetry / minidump creation in __except filters
    // (has nothing to do with C++ exceptions and doesn't require unwinding)

    // arg 0: exception code, must start with 0xE. "CCA": Clean Core Assert
    RaiseException(0xE0000CCA, 0, 0, nullptr);

    // NOTE: If uncaught, a simple _exit(3) quits much faster than this
    // NOTE: This does NOT trigger Windows Error Reporing (WER)
#else
    std::abort();
#endif
}

void cc::set_assertion_handler(void (*handler)(detail::assertion_info const&)) { s_current_handler = handler; }
