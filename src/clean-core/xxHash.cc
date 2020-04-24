#include "xxHash.hh"

#include <clean-core/detail/xxHash/xxh3.hh>

cc::hash_t cc::hash_xxh3(cc::span<const std::byte> data, cc::hash_t seed) { return XXH3_64bits_withSeed(data.data(), data.size(), seed); }
