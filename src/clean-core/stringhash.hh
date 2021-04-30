#pragma once

#include <clean-core/hash_combine.hh>

namespace cc
{
constexpr uint64_t stringhash(char const* s)
{
    if (!s)
        return 0;

    uint64_t h = hash_combine();
    while (*s)
    {
        h = hash_combine(h, uint64_t(*s));
        s++;
    }
    return h;
}

constexpr uint64_t stringhash_n(char const* s, int n)
{
    if (!s || n <= 0)
        return 0;

    uint64_t h = hash_combine();
    while (*s && n > 0)
    {
        h = hash_combine(h, uint64_t(*s));
        s++;
        n--;
    }
    return h;
}
}
