#pragma once

namespace cc
{
using int8 = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using float32 = float;
using float64 = double;

using byte = uint8;

using hash_t = uint64;
using size_t = int64;

using nullptr_t = decltype(nullptr);
} // namespace cc
