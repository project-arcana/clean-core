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
}
