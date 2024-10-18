#include "hash.xxh3.hh"

#include <clean-core/detail/xxHash/xxhash.hh>

uint64_t cc::make_hash_xxh3(cc::span<std::byte const> data, uint64_t seed) { return XXH3_64bits_withSeed(data.data(), data.size(), seed); }
