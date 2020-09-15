#pragma once

#include <clean-core/assert.hh>
#include <clean-core/format.hh>
#include <clean-core/to_string.hh>

// usage: CC_ASSERTF(a == b, "{} is not {}", a, b);
#define CC_ASSERTF(cond, ...) CC_ASSERT_MSG(cond, ::cc::format(__VA_ARGS__).c_str())
