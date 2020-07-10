#pragma once

#include <clean-core/macros.hh>
#ifdef CC_OS_WINDOWS

// Common Win32 entities forward declared
// NOTE: This header might cause conflicts if using Microsoft SAL code analysis tools
// See https://docs.microsoft.com/en-us/cpp/c-runtime-library/sal-annotations?redirectedfrom=MSDN&view=vs-2019

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef const wchar_t* LPCWSTR;
typedef void* HANDLE;
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HWND__* HWND;
typedef struct HMONITOR__* HMONITOR;
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
typedef struct _OVERLAPPED OVERLAPPED, *LPOVERLAPPED;
typedef struct _OVERLAPPED_ENTRY OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;
typedef long HRESULT;
typedef unsigned int UINT;

#ifdef _WIN64
typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
#else
#error "Unsupported platform"
#endif

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;

#endif
