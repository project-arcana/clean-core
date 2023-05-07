#pragma once

// NOTE: this header will be removed in a future version

#include <clean-core/hash.xxh3.hh>

namespace cc
{
[[deprecated("use cc::make_hash_xxh3 directly. this header will be removed in the future. the new name is <clean-core/hash.xxh3.hh> "
             "(2023-01-14)")]] inline uint64_t
hash_xxh3(cc::span<std::byte const> data, uint64_t seed)
{
    return cc::make_hash_xxh3(data, seed);
}
}
