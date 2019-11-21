#pragma once

#include <clean-core/macros.hh>
#ifdef CC_OS_WINDOWS

// Check if <Windows.h> was included somewhere before this header
#if defined(_WINDOWS_) && !defined(CC_SANITIZED_WINDOWS_H)
#pragma message("Including unsanitized Windows.h")
#endif
#define CC_SANITIZED_WINDOWS_H

// clang-format off
#include <clean-core/native/detail/win32_sanitize_before.inl>

#include <Windows.h>

#include <clean-core/native/detail/win32_sanitize_after.inl>
// clang-format on

#endif
