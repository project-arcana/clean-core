#pragma once

#include <cstdint>

namespace cc
{
[[nodiscard]] constexpr uint64_t hash_combine() noexcept { return 0x2a5114b5c6133408uLL; }
[[nodiscard]] constexpr uint64_t hash_combine(uint64_t a) noexcept { return a; }
[[nodiscard]] constexpr uint64_t hash_combine(uint64_t a, uint64_t b) noexcept { return a * 6364136223846793005ULL + b + 0xda3e39cb94b95bdbULL; }

template <class... Args>
[[nodiscard]] constexpr uint64_t hash_combine(uint64_t a, uint64_t b, uint64_t c, Args... rest) noexcept
{
    auto h = hash_combine(a, b);
    h = hash_combine(h, c);
    ((h = hash_combine(h, rest)), ...);
    return h;
}
}
