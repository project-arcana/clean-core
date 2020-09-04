#include "win32_util.hh"

#include <clean-core/macros.hh>

#ifdef CC_OS_WINDOWS

#include <clean-core/defer.hh>

#include <clean-core/native/win32_sanitized.hh>

bool cc::win32_get_version(unsigned& out_major, unsigned& out_minor, unsigned& out_build_number)
{
    // getting the current windows version is not as straightforward as it once was,
    // since GetVersion and GetVersionEx are deprecated and the Win32 Version Helper functions
    // will lie about the OS version depending on the manifest file.
    // They also give no direct information about the build number (which corresponds to "Windows 10 Versions"
    // like 1904, 1909, 2004, ..., or their marketing names like "Fall Creators Update", "May 2020 Update", ...)
    // which nowadays is often the only part of real interest

    // there are several approaches, this one is using the ancient,
    // undocumented NTDLL function RtlGetNtVersionNumbers
    // out parameters are "major number", "minor number" and "build number"
    // the build number must be masked to the lower 16 bit,
    // as the high bits contain additional information about it being a checked/free build

    // see
    //  http://www.geoffchappell.com/studies/windows/win32/ntdll/api/ldrinit/getntversionnumbers.htm
    //  https://stackoverflow.com/questions/47581146/getting-os-build-version-from-win32-api-c

    HMODULE hModule = GetModuleHandle(TEXT("ntdll.dll"));

    if (hModule == NULL)
        return false;

    typedef VOID(WINAPI * RtlGetNtVersionNumbersPtr)(DWORD*, DWORD*, DWORD*);
    auto const f_RtlGetNtVersionNumbers = (RtlGetNtVersionNumbersPtr)::GetProcAddress(hModule, "RtlGetNtVersionNumbers");

    if (f_RtlGetNtVersionNumbers == nullptr)
        return false;

    DWORD major_version, minor_version, build_number;
    f_RtlGetNtVersionNumbers(&major_version, &minor_version, &build_number);

    out_major = major_version;
    out_minor = minor_version;
    out_build_number = build_number & 0x0000FFFF;
    return true;
}

bool cc::win32_enable_schedular_granular()
{
    // Barely documented behavior of timeBeginPeriod,
    // sets the OS scheduler timeslice to the given value in milliseconds
    // in practice an argument of 1 ends up at ~.7ms
    // see:
    //   https://docs.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod
    //   https://hero.handmade.network/episode/code/day018/#3200

    // This change is global and should be undone at shutdown
    // It should not be called often (ideally just once)

    HMODULE hModWinmm = LoadLibrary("Winmm.dll");

    if (hModWinmm == NULL)
        return false;

    CC_DEFER { ::FreeLibrary(hModWinmm); };

    typedef UINT(WINAPI * timeBeginPeriodPtr)(_In_ UINT);
    auto const f_timeBeginPeriod = (timeBeginPeriodPtr)::GetProcAddress(hModWinmm, "timeBeginPeriod");

    if (f_timeBeginPeriod == nullptr)
        return false;

    return f_timeBeginPeriod(1) == 0 /*TIMERR_NOERROR*/;
}

bool cc::win32_disable_scheduler_granular()
{
    // Undos the change to the OS scheduler, the "period" specified must
    // be the same as in the first call

    HMODULE hModWinmm = LoadLibrary("Winmm.dll");

    if (hModWinmm == NULL)
        return false;

    CC_DEFER { ::FreeLibrary(hModWinmm); };

    typedef UINT(WINAPI * timeEndPeriodPtr)(_In_ UINT);
    auto const f_timeEndPeriod = (timeEndPeriodPtr)::GetProcAddress(hModWinmm, "timeEndPeriod");

    if (f_timeEndPeriod == nullptr)
        return false;

    return f_timeEndPeriod(1) == 0 /*TIMERR_NOERROR*/;
}


bool cc::win32_enable_console_colors()
{
    ::HANDLE const console_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (console_handle == INVALID_HANDLE_VALUE)
        return false;

    ::DWORD prev_mode;
    if (!::GetConsoleMode(console_handle, &prev_mode))
        return false;

    prev_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!::SetConsoleMode(console_handle, prev_mode))
        return false;

    return true;
}

#else

bool cc::win32_get_version(unsigned&, unsigned&, unsigned&) { return false; }
bool cc::win32_enable_schedular_granular() { return false; }
bool cc::win32_disable_scheduler_granular() { return false; }
bool cc::win32_enable_console_colors() { return false; }

#endif
