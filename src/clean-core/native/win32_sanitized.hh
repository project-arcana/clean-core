#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

// Check if <Windows.h> was included somewhere before this header
#if defined(_WINDOWS_) && !defined(CC_SANITIZED_WINDOWS_H)
#pragma message("[clean-core][native/win32_sanitized.hh] Detected unsanitized Windows.h")
#endif
#define CC_SANITIZED_WINDOWS_H

// clang-format off
#include <clean-core/native/detail/win32_sanitize_before.inl>

#include <Windows.h>

#include <clean-core/native/detail/win32_sanitize_after.inl>
// clang-format on

#endif
