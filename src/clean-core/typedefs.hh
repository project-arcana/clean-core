#pragma once

#include <cstddef>

namespace cc
{
using int8 = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = std::size_t;

using float32 = float;
using float64 = double;

using byte = std::byte;

using hash_t = uint64;
using size_t = std::size_t;

using nullptr_t = decltype(nullptr);

static_assert(sizeof(uint64) == sizeof(size_t), "only 64bit supported");
}
