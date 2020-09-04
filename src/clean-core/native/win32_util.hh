#pragma once

namespace cc
{
/// reads the true windows version using the undocumented RtlGetNtVersionNumbers NTDLL kernel function
///
/// output example on Win10 2004: { major: 10, minor: 0, build_number: 19041 }
/// for a list of build numbers and corresponding "versions" and marketing names, see
///   https://en.wikipedia.org/wiki/Windows_10_version_history#Channels
bool win32_get_version(unsigned& out_major, unsigned& out_minor, unsigned& out_build_number);

/// attempts to increase the OS scheduler timeslice to the minimum amount (~.7ms) using timeBeginPeriod
/// this change is global for the entire OS and should be undone at shutdown
bool win32_enable_schedular_granular();

/// undos the change made above
bool win32_disable_scheduler_granular();

/// enables ANSI Escape sequences in Windows conhost.exe and cmd.exe
bool win32_enable_console_colors();
}
