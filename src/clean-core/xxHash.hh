#pragma once

#include <cstddef>

#include <clean-core/span.hh>

namespace cc
{
// returns a hash of the data by executing https://github.com/Cyan4973/xxHash
cc::hash_t hash_xxh3(cc::span<std::byte const> data, cc::hash_t seed);
}
