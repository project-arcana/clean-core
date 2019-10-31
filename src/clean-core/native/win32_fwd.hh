#pragma once

#include <clean-core/macros.hh>
#ifdef CC_OS_WINDOWS

// Common Win32 entities forward declared
// NOTE: This header might cause conflicts if using Microsoft SAL code analysis tools
// See https://docs.microsoft.com/en-us/cpp/c-runtime-library/sal-annotations?redirectedfrom=MSDN&view=vs-2019

typedef unsigned long DWORD;
typedef const wchar_t* LPCWSTR;
typedef void* HANDLE;
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HWND__* HWND;
typedef struct HMONITOR__* HMONITOR;
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
typedef long HRESULT;

#endif
