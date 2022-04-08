#pragma once

#include <clean-core/macros.hh>

namespace cc
{
[[deprecated("use CC_DEBUG_BREAK()")]] CC_DONT_INLINE void breakpoint();
}
