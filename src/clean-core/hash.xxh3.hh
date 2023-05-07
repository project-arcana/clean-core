#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/span.hh>

namespace cc
{
// returns a hash of the data by executing https://github.com/Cyan4973/xxHash
uint64_t make_hash_xxh3(cc::span<std::byte const> data, uint64_t seed);
}
