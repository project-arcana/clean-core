#pragma once

#include <clean-core/hash_combine.hh>

namespace cc
{
constexpr hash_t stringhash(char const* s)
{
    if (!s)
        return 0;

    hash_t h = hash_combine();
    while (*s)
    {
        h = hash_combine(h, hash_t(*s));
        s++;
    }
    return h;
}

constexpr hash_t stringhash_n(char const* s, int n)
{
    if (!s || n <= 0)
        return 0;

    hash_t h = hash_combine();
    while (*s && n > 0)
    {
        h = hash_combine(h, hash_t(*s));
        s++;
        n--;
    }
    return h;
}
}
