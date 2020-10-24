#pragma once

namespace cc
{
/// reads the true windows version using the undocumented RtlGetNtVersionNumbers NTDLL kernel function
///
/// output example on Win10 2004: { major: 10, minor: 0, build_number: 19041 }
/// for a list of build numbers and corresponding "versions" and marketing names, see
///   https://en.wikipedia.org/wiki/Windows_10_version_history#Channels
bool win32_get_version(unsigned& out_major, unsigned& out_minor, unsigned& out_build_number);
}
