#pragma once

#include <cstddef>

#include <clean-core/assert.hh>
#include <clean-core/fwd.hh>

namespace cc
{
/// compile-time fixed-size bitset
/// TODO: select underlying type based on N
template <size_t N>
struct bitset
{
    static_assert(N <= 64, "TODO: support larger sizes");

    constexpr bitset() = default;
    constexpr explicit bitset(size_t data) : _data(data) {}

    // bit operations
public:
    friend constexpr bitset operator~(bitset const& a) { return bitset(~a._data); }

    friend constexpr bitset operator|(bitset const& a, bitset const& b) { return bitset(a._data | b._data); }
    friend constexpr bitset operator&(bitset const& a, bitset const& b) { return bitset(a._data & b._data); }
    friend constexpr bitset operator^(bitset const& a, bitset const& b) { return bitset(a._data ^ b._data); }
    friend constexpr bitset operator<<(bitset const& a, int b) { return bitset((a._data << b) & data_mask); }
    friend constexpr bitset operator>>(bitset const& a, int b) { return bitset(a._data >> b); }

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
        _data >>= b;
        return *this;
    }

    friend constexpr bool operator==(bitset const& a, bitset const& b) { return a._data == b._data; }
    friend constexpr bool operator!=(bitset const& a, bitset const& b) { return a._data != b._data; }
    // TODO: other comparisons?

    // properties
public:
    constexpr size_t size() const { return N; }
    // NOTE: no .data() ! this might get picked up wrongly by contiguous ranges

    constexpr bool any() const { return _data != 0; }
    constexpr bool all() const { return _data == data_mask; }

    constexpr bool is_set(size_t idx) const
    {
        CC_ASSERT(idx < N);
        return _data & (1 << idx);
    }
    constexpr bool is_unset(size_t idx) const
    {
        CC_ASSERT(idx < N);
        return !(_data & (1 << idx));
    }

    // NOTE: might change depending on N
    constexpr size_t representation() const { return _data; }

    // NOTE: currently not settable
    constexpr bool operator[](size_t idx) const
    {
        CC_ASSERT(idx < N);
        return _data & (1 << idx);
    }

    // methods
public:
    constexpr void clear() { _data = 0; }

    constexpr void set(size_t idx)
    {
        CC_ASSERT(idx < N);
        _data |= 1 << idx;
    }
    constexpr void unset(size_t idx)
    {
        CC_ASSERT(idx < N);
        _data &= ~(1 << idx);
    }
    constexpr void toggle(size_t idx)
    {
        CC_ASSERT(idx < N);
        _data ^= 1 << idx;
    }

private:
    // NOTE: always contains 0 in unused bits
    size_t _data = 0;

    static constexpr size_t data_mask = (1 << N) - 1;
};

/// dynamically allocated bitset
/// TODO: small buffer optimization!
template <>
struct bitset<dynamic_size>
{
    // TODO: implement me
};
}
