#include <clean-core/breakpoint.hh>

#ifdef CC_COMPILER_MSVC
#include <intrin.h>
#endif

// TODO: Maybe use IsDebuggerPresent (http://msdn.microsoft.com/en-us/library/ms680345%28VS.85%29.aspx)

void cc::breakpoint()
{
#if defined(CC_COMPILER_MSVC)
    __debugbreak();
#elif defined(CC_COMPILER_CLANG) || defined(CC_COMPILER_GCC)
    __builtin_trap();
#else
#error "Unknown compiler"
#endif
}
