#pragma once

#include <cstddef>

#include <clean-core/assert.hh>
#include <clean-core/bits.hh>
#include <clean-core/fwd.hh>

namespace cc
{
namespace detail
{
/// a simple wrapper for W x u64
/// where only "bit operations" are exposed
template <int W>
struct bit_words
{
    static constexpr int word_count = W;

    uint64_t words[W] = {};

    constexpr bit_words() = default;
    // NOTE: only lower word!
    constexpr explicit bit_words(uint64_t data) { words[0] = data; }

    static constexpr bit_words ones(size_t cnt)
    {
        // CC_ASSERT(cnt < W * 8 * sizeof(uint64_t)); -- not constexpr
        bit_words w;
        size_t i = 0;

        while (cnt >= 64)
        {
            w.words[i] = uint64_t(-1);
            ++i;
            cnt -= 64;
        }

        if (cnt > 0)
            w.words[i] = (uint64_t(1) << cnt) - 1;

        return w;
    }

    friend constexpr bit_words operator~(bit_words ws)
    {
        for (auto& w : ws.words)
            w = ~w;
        return ws;
    }

    friend constexpr bit_words operator|(bit_words const& ws0, bit_words const& ws1)
    {
        bit_words w;
        for (auto i = 0; i < W; ++i)
            w.words[i] = ws0.words[i] | ws1.words[i];
        return w;
    }

    friend constexpr bit_words operator&(bit_words const& ws0, bit_words const& ws1)
    {
        bit_words w;
        for (auto i = 0; i < W; ++i)
            w.words[i] = ws0.words[i] & ws1.words[i];
        return w;
    }

    friend constexpr bit_words operator^(bit_words const& ws0, bit_words const& ws1)
    {
        bit_words w;
        for (auto i = 0; i < W; ++i)
            w.words[i] = ws0.words[i] ^ ws1.words[i];
        return w;
    }

    friend constexpr bit_words operator>>(bit_words const& ws, int b)
    {
        auto w_offset = b / 64;
        auto w_shift = b - 64 * w_offset;
        if (w_shift < 0)
        {
            w_offset--;
            w_shift += 64;
        }
        CC_ASSERT(0 <= w_shift && w_shift < 64);

        bit_words w;
        for (auto i = 0; i < W; ++i)
        {
            auto wi = i + w_offset;
            auto wii = wi + 1;

            if (0 <= wi && wi < W)
                w.words[i] = ws.words[wi] >> w_shift;

            if (w_shift > 0 && 0 <= wii && wii < W)
                w.words[i] |= ws.words[wii] << (64 - w_shift);
        }
        return w;
    }

    friend constexpr bit_words operator<<(bit_words const& ws, int b) { return ws >> -b; }

    constexpr bit_words& operator|=(bit_words const& ws)
    {
        for (auto i = 0; i < W; ++i)
            words[i] |= ws.words[i];
        return *this;
    }

    constexpr bit_words& operator&=(bit_words const& ws)
    {
        for (auto i = 0; i < W; ++i)
            words[i] &= ws.words[i];
        return *this;
    }

    constexpr bit_words& operator^=(bit_words const& ws)
    {
        for (auto i = 0; i < W; ++i)
            words[i] ^= ws.words[i];
        return *this;
    }

    constexpr bit_words& operator<<=(int b)
    {
        *this = *this << b;
        return *this;
    }

    constexpr bit_words& operator>>=(int b)
    {
        *this = *this >> b;
        return *this;
    }

    friend constexpr bool operator==(bit_words const& ws0, bit_words const& ws1)
    {
        for (auto i = 0; i < W; ++i)
            if (ws0.words[i] != ws1.words[i])
                return false;
        return true;
    }

    friend constexpr bool operator!=(bit_words const& ws0, bit_words const& ws1)
    {
        for (auto i = 0; i < W; ++i)
            if (ws0.words[i] != ws1.words[i])
                return true;
        return false;
    }
};
} // namespace detail

/// compile-time fixed-size bitset
/// TODO: optimize some stuff if N % 64 == 0
template <size_t N>
struct bitset
{
    static_assert(N > 0, "TODO: support zero-sized bitset for compat purposes");
    using repr_t = detail::bit_words<1 + (N - 1) / (8 * sizeof(uint64_t))>;

    constexpr bitset() = default;
    constexpr explicit bitset(uint64_t data) : _data(data) {}
    constexpr explicit bitset(repr_t data) : _data(data) {}

    // ctor
public:
    static constexpr bitset zeroes() { return bitset(); }
    static constexpr bitset ones() { return bitset(data_mask); }
    static constexpr bitset filled(bool value) { return value ? bitset(data_mask) : bitset(); }

    static constexpr bitset ones(int n)
    {
        CC_ASSERT(0 <= n && n <= int(N));
        bitset b;
        int i = 0;
        while (n >= 64)
        {
            b._data.words[i] = uint64_t(-1);
            n -= 64;
            i++;
        }
        if (n > 0)
            b._data.words[i] = (uint64_t(1) << n) - 1;
        return b;
    }

    // bit operations
public:
    friend constexpr bitset operator~(bitset const& a) { return bitset(~a._data & data_mask); }

    friend constexpr bitset operator|(bitset const& a, bitset const& b) { return bitset(a._data | b._data); }
    friend constexpr bitset operator&(bitset const& a, bitset const& b) { return bitset(a._data & b._data); }
    friend constexpr bitset operator^(bitset const& a, bitset const& b) { return bitset(a._data ^ b._data); }
    friend constexpr bitset operator<<(bitset const& a, int b) { return bitset((a._data << b) & data_mask); }
    friend constexpr bitset operator>>(bitset const& a, int b) { return bitset((a._data >> b) & data_mask); }

    constexpr bitset& operator|=(bitset const& b)
    {
        _data |= b._data;
        return *this;
    }
    constexpr bitset& operator&=(bitset const& b)
    {
        _data &= b._data;
        return *this;
    }
    constexpr bitset& operator^=(bitset const& b)
    {
        _data ^= b._data;
        return *this;
    }
    constexpr bitset& operator<<=(int b)
    {
        _data = (_data << b) & data_mask; // ensure no 1 in upper bits
        return *this;
    }
    constexpr bitset& operator>>=(int b)
    {
        _data = (_data >> b) & data_mask; // ensure no 1 in upper bits (for negative b)
        return *this;
    }

    friend constexpr bool operator==(bitset const& a, bitset const& b) { return a._data == b._data; }
    friend constexpr bool operator!=(bitset const& a, bitset const& b) { return a._data != b._data; }

    // properties
public:
    constexpr size_t size() const { return N; }
    // NOTE: no .data() ! this might get picked up wrongly by contiguous ranges

    constexpr bool any() const { return _data != repr_t{}; }
    constexpr bool all() const { return _data == data_mask; }

    constexpr bool is_set(size_t idx) const
    {
        CC_ASSERT(idx < N);
        return _data.words[idx / 64] & (uint64_t(1) << (idx % 64));
    }
    constexpr bool is_unset(size_t idx) const
    {
        CC_ASSERT(idx < N);
        return !(_data.words[idx / 64] & (uint64_t(1) << (idx % 64)));
    }

    constexpr repr_t const& representation() const { return _data; }

    // NOTE: currently not settable
    constexpr bool operator[](size_t idx) const
    {
        CC_ASSERT(idx < N);
        return _data.words[idx / 64] & (uint64_t(1) << (idx % 64));
    }

    int count_trailing_zeroes() const
    {
        auto cnt = 0;
        for (auto i = 0; i < _data.word_count; ++i)
        {
            if (_data.words[i] != 0)
                return cnt + cc::count_trailing_zeros(_data.words[i]);

            cnt += 64;
        }
        return N;
    }

    // methods
public:
    constexpr void clear() { _data = {}; }

    constexpr void set(size_t idx)
    {
        CC_ASSERT(idx < N);
        _data.words[idx / 64] |= uint64_t(1) << (idx % 64);
    }
    constexpr void unset(size_t idx)
    {
        CC_ASSERT(idx < N);
        _data.words[idx / 64] &= ~(uint64_t(1) << (idx % 64));
    }
    constexpr void toggle(size_t idx)
    {
        CC_ASSERT(idx < N);
        _data.words[idx / 64] ^= uint64_t(1) << (idx % 64);
    }

private:
    // NOTE: always contains 0 in unused bits
    repr_t _data = {};

    static constexpr repr_t data_mask = repr_t::ones(N);
};

/// dynamically allocated bitset
/// TODO: small buffer optimization!
template <>
struct bitset<dynamic_size>
{
    // TODO: implement me
};
} // namespace cc
