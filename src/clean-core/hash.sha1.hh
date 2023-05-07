#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/array.hh>
#include <clean-core/span.hh>
#include <clean-core/string.hh>

// TODO: version where data can be streamed in

namespace cc
{
// returns a hash of the data by executing SHA1
cc::array<std::byte, 20> make_hash_sha1(cc::string_view data);
cc::array<std::byte, 20> make_hash_sha1(cc::span<std::byte const> data);
// same as make_hash_sha1, but the result is a hex string (length is 40)
cc::string make_hash_sha1_string(cc::string_view data);
cc::string make_hash_sha1_string(cc::span<std::byte const> data);
}
