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

#else

bool cc::win32_get_version(unsigned&, unsigned&, unsigned&) { return false; }

#endif
