#include "hash.sha1.hh"

cc::array<std::byte, 20> cc::make_hash_sha1(cc::span<const std::byte> data)
{
    sha1_builder sha1;

    sha1.reset();
    sha1.add(data);
    return sha1.finalize();
}

cc::array<std::byte, 20> cc::make_hash_sha1(string_view data) { return cc::make_hash_sha1(cc::as_byte_span(data)); }

cc::string cc::make_hash_sha1_string(cc::span<const std::byte> data)
{
    auto hex = "0123456789abcdef";
    auto hash = cc::make_hash_sha1(data);
    auto s = cc::string::uninitialized(40);
    for (auto i = 0; i < 20; ++i)
    {
        s[i * 2 + 0] = hex[uint8_t(hash[i]) >> 4];
        s[i * 2 + 1] = hex[uint8_t(hash[i]) & 0b1111];
    }
    return s;
}

cc::string cc::make_hash_sha1_string(string_view data) { return cc::make_hash_sha1_string(cc::as_byte_span(data)); }
