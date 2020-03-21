#pragma once

#include <clean-core/typedefs.hh>

namespace cc
{
constexpr hash_t hash_combine() noexcept { return 0x2a5114b5c6133408uLL; }
constexpr hash_t hash_combine(hash_t a) noexcept { return a; }
constexpr hash_t hash_combine(hash_t a, hash_t b) noexcept { return a * 6364136223846793005ULL + b + 0xda3e39cb94b95bdbULL; }

template <class... Args>
constexpr hash_t hash_combine(hash_t a, hash_t b, hash_t c, Args... rest) noexcept
{
    auto h = hash_combine(a, b);
    h = hash_combine(h, c);
    ((h = hash_combine(h, rest)), ...);
    return h;
}
}