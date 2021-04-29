#include "xxHash.hh"

#include <clean-core/detail/xxHash/xxh3.hh>

uint64_t cc::hash_xxh3(cc::span<const std::byte> data, uint64_t seed) { return XXH3_64bits_withSeed(data.data(), data.size(), seed); }
