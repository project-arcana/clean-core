#include <nexus/monte_carlo_test.hh>

#include <clean-core/bitset.hh>
#include <clean-core/indices_of.hh>
#include <clean-core/set.hh>

// NOTE: bit operations are often subtly wrong, so we really test the whole interface

namespace
{

template <int N>
struct reference_set
{
    cc::set<int> entries;

    static reference_set zeroes() { return {}; }
    static reference_set ones()
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            s.entries.add(i);
        return s;
    }
    static reference_set ones(int n)
    {
        CC_ASSERT(0 <= n && n <= N);
        reference_set s;
        for (auto i : cc::indices_of(n))
            s.entries.add(i);
        return s;
    }
    static reference_set filled(bool value)
    {
        reference_set s;
        if (value)
            for (auto i : cc::indices_of(N))
                s.entries.add(i);
        return s;
    }

    size_t size() const { return N; }
    bool any() const { return !entries.empty(); }
    bool all() const { return entries.size() == N; }

    bool is_set(size_t idx) const { return entries.contains(idx); }
    bool is_unset(size_t idx) const { return !entries.contains(idx); }

    bool operator[](size_t idx) const { return entries.contains(idx); }

    void clear() { entries.clear(); }

    void set(size_t idx) { entries.add(idx); }
    void unset(size_t idx) { entries.remove(idx); }

    void toggle(size_t idx)
    {
        if (entries.contains(idx))
            entries.remove(idx);
        else
            entries.add(idx);
    }

    int count_trailing_zeroes() const
    {
        for (auto i : cc::indices_of(N))
            if (entries.contains(i))
                return i;
        return N;
    }

    friend reference_set operator~(reference_set const& a)
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            if (!a.entries.contains(i))
                s.entries.add(i);
        return s;
    }

    friend reference_set operator|(reference_set const& a, reference_set const& b)
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            if (a.entries.contains(i) || b.entries.contains(i))
                s.entries.add(i);
        return s;
    }

    friend reference_set operator&(reference_set const& a, reference_set const& b)
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            if (a.entries.contains(i) && b.entries.contains(i))
                s.entries.add(i);
        return s;
    }

    friend reference_set operator^(reference_set const& a, reference_set const& b)
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            if (a.entries.contains(i) != b.entries.contains(i))
                s.entries.add(i);
        return s;
    }

    friend reference_set operator<<(reference_set const& a, int b)
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            if (a.entries.contains(i - b))
                s.entries.add(i);
        return s;
    }

    friend reference_set operator>>(reference_set const& a, int b)
    {
        reference_set s;
        for (auto i : cc::indices_of(N))
            if (a.entries.contains(i + b))
                s.entries.add(i);
        return s;
    }

    reference_set& operator|=(reference_set const& a)
    {
        if (&a == this)
            return *this;

        for (auto i : a.entries)
            entries.add(i);
        return *this;
    }

    reference_set& operator&=(reference_set const& a)
    {
        if (&a == this)
            return *this;

        for (auto i : cc::indices_of(N))
            if (!a.entries.contains(i))
                entries.remove(i);
        return *this;
    }

    reference_set& operator^=(reference_set const& a)
    {
        cc::set<int> new_entries;
        for (auto i : cc::indices_of(N))
            if (entries.contains(i) != a.entries.contains(i))
                new_entries.add(i);
        entries = cc::move(new_entries);
        return *this;
    }

    reference_set& operator<<=(int b)
    {
        cc::set<int> new_entries;
        for (auto i : cc::indices_of(N))
            if (entries.contains(i - b))
                new_entries.add(i);
        entries = cc::move(new_entries);
        return *this;
    }

    reference_set& operator>>=(int b)
    {
        cc::set<int> new_entries;
        for (auto i : cc::indices_of(N))
            if (entries.contains(i + b))
                new_entries.add(i);
        entries = cc::move(new_entries);
        return *this;
    }

    bool operator==(reference_set const& b) const { return entries == b.entries; }
    bool operator!=(reference_set const& b) const { return entries != b.entries; }

    bool operator==(cc::bitset<N> const& b) const
    {
        for (auto i : cc::indices_of(N))
            if (entries.contains(i) != b.is_set(i))
                return false;
        return true;
    }
    bool operator!=(cc::bitset<N> const& b) const
    {
        for (auto i : cc::indices_of(N))
            if (entries.contains(i) != b.is_set(i))
                return true;
        return false;
    }

    friend bool operator==(cc::bitset<N> const& a, reference_set const& b) { return b == a; }
    friend bool operator!=(cc::bitset<N> const& a, reference_set const& b) { return b != a; }
};

} // namespace

MONTE_CARLO_TEST("cc::bitset mct")
{
    addValue("int 0", 0);
    addValue("int 1", 1);
    addValue("int -1", -1);
    addValue("int 10", 10);
    addValue("int -10", -10);
    addValue("int 63", 63);
    addValue("int 64", 64);
    addValue("int 65", 65);

    addOp("rng int 10", [](tg::rng& rng) { return uniform(rng, -10, 10); });
    addOp("rng int 100", [](tg::rng& rng) { return uniform(rng, -100, 100); });
    addOp("rng int 200", [](tg::rng& rng) { return uniform(rng, -200, 200); });

    auto setup_type = [this](auto bs)
    {
        using bitset_t = decltype(bs);

        setPrinter<bitset_t>(
            [](bitset_t const& b)
            {
                auto s = cc::string::filled(b.size(), '0');
                for (auto i : cc::indices_of(b.size()))
                    if (b[b.size() - i - 1])
                        s[i] = '1';
                return s;
            });

        auto const idx_in_range = [s = int(bs.size())](int i) { return 0 <= i && i < s; };

        addValue("default", bitset_t{});
        addValue("zeroes", bitset_t::zeroes());
        addValue("ones", bitset_t::ones());
        addValue("filled(true)", bitset_t::filled(true));
        addValue("filled(false)", bitset_t::filled(false));

        addOp("op~", [](bitset_t const& b) { return ~b; });
        addOp("op|", [](bitset_t const& b0, bitset_t const& b1) { return b0 | b1; });
        addOp("op&", [](bitset_t const& b0, bitset_t const& b1) { return b0 & b1; });
        addOp("op^", [](bitset_t const& b0, bitset_t const& b1) { return b0 ^ b1; });
        addOp("op<<", [](bitset_t const& b0, int b) { return b0 << b; });
        addOp("op>>", [](bitset_t const& b0, int b) { return b0 >> b; });

        addOp("op|=", [](bitset_t& b0, bitset_t const& b1) { b0 |= b1; });
        addOp("op&=", [](bitset_t& b0, bitset_t const& b1) { b0 &= b1; });
        addOp("op^=", [](bitset_t& b0, bitset_t const& b1) { b0 ^= b1; });
        addOp("op<<=", [](bitset_t& b0, int b) { b0 <<= b; });
        addOp("op>>=", [](bitset_t& b0, int b) { b0 >>= b; });

        addOp("any", [](bitset_t const& b0) { return b0.any(); });
        addOp("all", [](bitset_t const& b0) { return b0.all(); });

        addOp("ctz", [](bitset_t const& b0) { return b0.count_trailing_zeroes(); });

        addOp("is_set", [](bitset_t const& b0, int b) { return b0.is_set(b); }).when(idx_in_range);
        addOp("is_unset", [](bitset_t const& b0, int b) { return b0.is_unset(b); }).when(idx_in_range);
        addOp("op[]", [](bitset_t const& b0, int b) { return b0[b]; }).when(idx_in_range);

        addOp("clear", [](bitset_t& b0) { b0.clear(); });
        addOp("set", [](bitset_t& b0, int b) { b0.set(b); }).when(idx_in_range);
        addOp("unset", [](bitset_t& b0, int b) { b0.unset(b); }).when(idx_in_range);
        addOp("toggle", [](bitset_t& b0, int b) { b0.toggle(b); }).when(idx_in_range);

        addOp("ones_n", [](int b) { return bitset_t::ones(b); }).when(idx_in_range);
    };

    setup_type(cc::bitset<64>{});
    setup_type(reference_set<64>{});
    testEquivalence<cc::bitset<64>, reference_set<64>>();

    setup_type(cc::bitset<128>{});
    setup_type(reference_set<128>{});
    testEquivalence<cc::bitset<128>, reference_set<128>>();

    setup_type(cc::bitset<63>{});
    setup_type(reference_set<63>{});
    testEquivalence<cc::bitset<63>, reference_set<63>>();

    setup_type(cc::bitset<65>{});
    setup_type(reference_set<65>{});
    testEquivalence<cc::bitset<65>, reference_set<65>>();

    setup_type(cc::bitset<130>{});
    setup_type(reference_set<130>{});
    testEquivalence<cc::bitset<130>, reference_set<130>>();
}

TEST("cc::bitset cases")
{
    {
        auto bs = cc::bitset<64>::zeroes();
        bs.toggle(63);
        CHECK(bs.representation().words[0] == (size_t(1) << 63));
    }
    {
        auto bs = cc::bitset<63>::ones();
        bs = bs >> 63;
        bs = ~bs;
        CHECK(bs.all());
    }
}
