#include "hash.sha1.hh"

#include <clean-core/bits.hh>

// TODO: expose stream api at some point
namespace
{
// impl loosely based on https://github.com/vog/sha1 (public domain)
// and https://en.wikipedia.org/wiki/SHA-1
// with significant performance improvements
struct sha1_builder
{
    static constexpr size_t BLOCK_INTS = 16; /* number of 32bit integers per SHA1 block */
    static constexpr size_t BLOCK_BYTES = BLOCK_INTS * 4;

    uint32_t digest[5];
    size_t total_size_in_bytes;

    // tmp buffer
    // size is 2 buffers so we can compute padding easily
    std::byte buffer_block[BLOCK_BYTES * 2];
    size_t buffer_size;

    static uint32_t rol(const uint32_t value, const size_t bits) { return (value << bits) | (value >> (32 - bits)); }
    static uint32_t blk(const uint32_t block[BLOCK_INTS], const size_t i)
    {
        return rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i], 1);
    }

    /*
     * (R0+R1), R2, R3, R4 are the different operations used in SHA1
     */

    static void R0(const uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i)
    {
        z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
        w = rol(w, 30);
    }

    static void R1(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i)
    {
        block[i] = blk(block, i);
        z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
        w = rol(w, 30);
    }

    static void R2(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i)
    {
        block[i] = blk(block, i);
        z += (w ^ x ^ y) + block[i] + 0x6ed9eba1 + rol(v, 5);
        w = rol(w, 30);
    }

    static void R3(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i)
    {
        block[i] = blk(block, i);
        z += (((w | x) & y) | (w & x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
        w = rol(w, 30);
    }

    static void R4(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i)
    {
        block[i] = blk(block, i);
        z += (w ^ x ^ y) + block[i] + 0xca62c1d6 + rol(v, 5);
        w = rol(w, 30);
    }


    /*
     * Hash a single 512-bit block. This is the core of the algorithm.
     */
    void transform(std::byte buffer[BLOCK_BYTES])
    {
        // convert endianness
        auto buffer_u32 = (uint32_t*)buffer;
        uint32_t block[BLOCK_INTS];
        for (size_t i = 0; i < BLOCK_INTS; ++i)
            block[i] = cc::byteswap(buffer_u32[i]);

        /* Copy digest[] to working vars */
        uint32_t a = digest[0];
        uint32_t b = digest[1];
        uint32_t c = digest[2];
        uint32_t d = digest[3];
        uint32_t e = digest[4];

        /* 4 rounds of 20 operations each. Loop unrolled. */
        R0(block, a, b, c, d, e, 0);
        R0(block, e, a, b, c, d, 1);
        R0(block, d, e, a, b, c, 2);
        R0(block, c, d, e, a, b, 3);
        R0(block, b, c, d, e, a, 4);
        R0(block, a, b, c, d, e, 5);
        R0(block, e, a, b, c, d, 6);
        R0(block, d, e, a, b, c, 7);
        R0(block, c, d, e, a, b, 8);
        R0(block, b, c, d, e, a, 9);
        R0(block, a, b, c, d, e, 10);
        R0(block, e, a, b, c, d, 11);
        R0(block, d, e, a, b, c, 12);
        R0(block, c, d, e, a, b, 13);
        R0(block, b, c, d, e, a, 14);
        R0(block, a, b, c, d, e, 15);
        R1(block, e, a, b, c, d, 0);
        R1(block, d, e, a, b, c, 1);
        R1(block, c, d, e, a, b, 2);
        R1(block, b, c, d, e, a, 3);
        R2(block, a, b, c, d, e, 4);
        R2(block, e, a, b, c, d, 5);
        R2(block, d, e, a, b, c, 6);
        R2(block, c, d, e, a, b, 7);
        R2(block, b, c, d, e, a, 8);
        R2(block, a, b, c, d, e, 9);
        R2(block, e, a, b, c, d, 10);
        R2(block, d, e, a, b, c, 11);
        R2(block, c, d, e, a, b, 12);
        R2(block, b, c, d, e, a, 13);
        R2(block, a, b, c, d, e, 14);
        R2(block, e, a, b, c, d, 15);
        R2(block, d, e, a, b, c, 0);
        R2(block, c, d, e, a, b, 1);
        R2(block, b, c, d, e, a, 2);
        R2(block, a, b, c, d, e, 3);
        R2(block, e, a, b, c, d, 4);
        R2(block, d, e, a, b, c, 5);
        R2(block, c, d, e, a, b, 6);
        R2(block, b, c, d, e, a, 7);
        R3(block, a, b, c, d, e, 8);
        R3(block, e, a, b, c, d, 9);
        R3(block, d, e, a, b, c, 10);
        R3(block, c, d, e, a, b, 11);
        R3(block, b, c, d, e, a, 12);
        R3(block, a, b, c, d, e, 13);
        R3(block, e, a, b, c, d, 14);
        R3(block, d, e, a, b, c, 15);
        R3(block, c, d, e, a, b, 0);
        R3(block, b, c, d, e, a, 1);
        R3(block, a, b, c, d, e, 2);
        R3(block, e, a, b, c, d, 3);
        R3(block, d, e, a, b, c, 4);
        R3(block, c, d, e, a, b, 5);
        R3(block, b, c, d, e, a, 6);
        R3(block, a, b, c, d, e, 7);
        R3(block, e, a, b, c, d, 8);
        R3(block, d, e, a, b, c, 9);
        R3(block, c, d, e, a, b, 10);
        R3(block, b, c, d, e, a, 11);
        R4(block, a, b, c, d, e, 12);
        R4(block, e, a, b, c, d, 13);
        R4(block, d, e, a, b, c, 14);
        R4(block, c, d, e, a, b, 15);
        R4(block, b, c, d, e, a, 0);
        R4(block, a, b, c, d, e, 1);
        R4(block, e, a, b, c, d, 2);
        R4(block, d, e, a, b, c, 3);
        R4(block, c, d, e, a, b, 4);
        R4(block, b, c, d, e, a, 5);
        R4(block, a, b, c, d, e, 6);
        R4(block, e, a, b, c, d, 7);
        R4(block, d, e, a, b, c, 8);
        R4(block, c, d, e, a, b, 9);
        R4(block, b, c, d, e, a, 10);
        R4(block, a, b, c, d, e, 11);
        R4(block, e, a, b, c, d, 12);
        R4(block, d, e, a, b, c, 13);
        R4(block, c, d, e, a, b, 14);
        R4(block, b, c, d, e, a, 15);

        /* Add the working vars back into digest[] */
        digest[0] += a;
        digest[1] += b;
        digest[2] += c;
        digest[3] += d;
        digest[4] += e;
    }


    void reset()
    {
        /* SHA1 initialization constants */
        digest[0] = 0x67452301;
        digest[1] = 0xefcdab89;
        digest[2] = 0x98badcfe;
        digest[3] = 0x10325476;
        digest[4] = 0xc3d2e1f0;

        /* Reset counters */
        total_size_in_bytes = 0;
        buffer_size = 0;
    }

    void update(cc::span<std::byte const> data)
    {
        total_size_in_bytes += data.size_bytes();

        while (true)
        {
            auto bytes_for_block = BLOCK_BYTES - buffer_size;

            // not enough data for whole block?
            if (bytes_for_block > data.size())
            {
                std::memcpy(buffer_block + buffer_size, data.data(), data.size());
                buffer_size += data.size();
                CC_ASSERT(buffer_size < BLOCK_BYTES);
                return;
            }

            // fill block
            std::memcpy(buffer_block + buffer_size, data.data(), bytes_for_block);
            data = data.subspan(bytes_for_block);
            transform(buffer_block);
            buffer_size = 0;
        }
    }

    /*
     * Add padding and return the message digest.
     */
    [[nodiscard]] cc::array<std::byte, 20> finalize()
    {
        /* Total number of hashed bits */
        uint64_t total_bits = total_size_in_bytes * 8;

        /* Padding */
        // append the bit '1' to the message e.g. by adding 0x80 if message length is a multiple of 8 bits.
        buffer_block[buffer_size++] = (std::byte)0x80;

        // append 0 <= k < 512 bits '0', such that the resulting message length in bits is congruent to −64 === 448 (mod 512)
        while (buffer_size % 64 != 56)
            buffer_block[buffer_size++] = (std::byte)0x00;

        // append ml, the original message length in bits, as a 64-bit big-endian integer.
        // NOTE: could save 4 byteswaps by doing this in transform
        *(uint64_t*)(buffer_block + buffer_size) = cc::byteswap(total_bits);
        buffer_size += 8;
        CC_ASSERT(buffer_size == BLOCK_BYTES || buffer_size == 2 * BLOCK_BYTES);

        // update buffers
        transform(buffer_block);
        if (buffer_size == 2 * BLOCK_BYTES)
            transform(buffer_block + BLOCK_BYTES);

        // build result
        digest[0] = cc::byteswap(digest[0]);
        digest[1] = cc::byteswap(digest[1]);
        digest[2] = cc::byteswap(digest[2]);
        digest[3] = cc::byteswap(digest[3]);
        digest[4] = cc::byteswap(digest[4]);
        cc::array<std::byte, 20> res;
        cc::as_byte_span(digest).copy_to(res);

        return res;
    }
};
}

cc::array<std::byte, 20> cc::make_hash_sha1(cc::span<const std::byte> data)
{
    sha1_builder sha1;

    sha1.reset();
    sha1.update(data);
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
