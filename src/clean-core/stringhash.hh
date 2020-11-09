#pragma once

#include <clean-core/typedefs.hh>

namespace cc
{
/// calculate a string hash from a string literal at compile time
template <size_t N>
constexpr hash_t stringhash(const char (&str)[N], size_t prime = 31, size_t length = N - 1)
{
    // ref https://xueyouchao.github.io/2016/11/16/CompileTimeString/
    return (length <= 1) ? str[0] : (prime * stringhash(str, prime, length - 1) + str[length - 1]);
}

/// usage:
static_assert(stringhash("value") != 0);

/// calculate a string hash from a runtime string
inline hash_t stringhash_runtime(char const* str, size_t prime = 31)
{
    if (str == nullptr)
        return 0;
    hash_t hash = *str;
    for (; *(str + 1) != 0; str++)
    {
        hash = prime * hash + *(str + 1);
    }
    return hash;
}
}
