#include <nexus/app.hh>

#undef RICH_LOG_FORCE_MACRO_PREFIX
#include <rich-log/log.hh>

#include <iostream>
#include <set>
#include <tuple>

#include <clean-core/format.hh>
#include <clean-core/hash.hh>
#include <clean-core/map.hh>
#include <clean-core/set.hh>
#include <clean-core/tuple.hh>
#include <clean-core/xxHash.hh>

#include <typed-geometry/feature/random.hh>

namespace
{
//
// ???
//
size_t hash_combine_simplexor(size_t a, size_t b, size_t c) { return ((a ^ (b << 1)) >> 1) ^ (c << 1); }
size_t hash_combine_simplexor(size_t a, size_t b) { return ((a ^ (b << 1)) >> 1); }

//
// cc
//
template <class... Args>
size_t hash_combine_cc(Args... h)
{
    return cc::hash_combine(h...);
}

//
// boost
//
size_t hash_combine_boost(size_t seed, size_t h) { return seed ^ (h + 0x9e3779b97f4a7c15uLL + (seed << 6) + (seed >> 2)); }
template <class... Args>
size_t hash_combine_boost(size_t a, size_t b, size_t c, Args... h)
{
    return hash_combine_boost(a, hash_combine_boost(b, c, h...));
}

//
// xxHash
//
template <class... Args>
size_t hash_combine_xxHash(Args... h)
{
    size_t data[] = {h...};
    return cc::make_hash_xxh3(cc::as_byte_span(data), 0xDEADBEEF);
}
}

APP("hash comparison")
{
    auto make_rand_u32 = [](tg::rng& rng) -> uint32_t { return rng() & 0xFF'FF'FF'FF; };
    auto make_rand_i32 = [](tg::rng& rng) -> int32_t { return int32_t(rng() & 0xFF'FF'FF'FF); };
    auto make_rand_u64 = [](tg::rng& rng) -> uint64_t { return (uint64_t(rng()) & 0xFF'FF'FF'FFuLL) | ((uint64_t(rng()) & 0xFF'FF'FF'FFuLL) << 32); };
    auto make_rand_i64 = [](tg::rng& rng) -> int64_t { return (uint64_t(rng()) & 0xFF'FF'FF'FFuLL) | ((uint64_t(rng()) & 0xFF'FF'FF'FFuLL) << 32); };
    auto make_rand_float = [](tg::rng& rng) -> float { return uniform(rng, -1000.f, 1000.f); };
    auto make_rand_double = [](tg::rng& rng) -> double { return uniform(rng, -1000., 1000.); };
    auto make_small_nat32 = [](tg::rng& rng) -> int32_t { return uniform(rng, 0, 100); };
    auto make_small_int32 = [](tg::rng& rng) -> int32_t { return uniform(rng, -100, 100); };
    auto make_small_nat64 = [](tg::rng& rng) -> int64_t { return uniform(rng, 0, 100); };
    auto make_small_int64 = [](tg::rng& rng) -> int64_t { return uniform(rng, -100, 100); };
    auto make_small_nat_float = [](tg::rng& rng) -> float { return uniform(rng, 0, 100); };
    auto make_small_int_float = [](tg::rng& rng) -> float { return uniform(rng, -100, 100); };
    auto make_small_nat_double = [](tg::rng& rng) -> double { return uniform(rng, 0, 100); };
    auto make_small_int_double = [](tg::rng& rng) -> double { return uniform(rng, -100, 100); };
    auto make_float_01 = [](tg::rng& rng) -> float { return uniform(rng, 0.f, 1.f); };
    auto make_float_m11 = [](tg::rng& rng) -> float { return uniform(rng, -1.f, 1.f); };
    auto make_double_01 = [](tg::rng& rng) -> double { return uniform(rng, 0., 1.); };
    auto make_double_m11 = [](tg::rng& rng) -> double { return uniform(rng, -1., 1.); };
    auto make_char = [](tg::rng& rng) { return uniform(rng, ' ', '~'); };

    struct test_case
    {
        cc::string name;
        cc::vector<uint64_t> hashes;
        int arity = 0;
    };
    auto make_test_set = [&](cc::string_view name, int count, int arity, auto&& make_elem)
    {
        tg::rng rng;
        using T = std::decay_t<decltype(make_elem(rng))>;
        test_case test;
        test.name = name;
        test.arity = arity;
        std::set<std::tuple<size_t, size_t, size_t>> seen;
        auto retries = 0;
        while (int(test.hashes.size()) < count * arity && retries < 1000)
        {
            auto h1 = cc::hash<T>()(make_elem(rng));
            auto h2 = arity >= 2 ? cc::hash<T>()(make_elem(rng)) : 0;
            auto h3 = arity >= 3 ? cc::hash<T>()(make_elem(rng)) : 0;

            auto t = std::tuple{h1, h2, h3};

            if (seen.count(t))
            {
                retries++;
                continue;
            }

            seen.insert(t);

            if (arity >= 1)
                test.hashes.push_back(h1);
            if (arity >= 2)
                test.hashes.push_back(h2);
            if (arity >= 3)
                test.hashes.push_back(h3);

            retries = 0;
        }

        return test;
    };

    cc::vector<test_case> tests;

    auto make_test_set_23 = [&](cc::string_view name, int count, auto&& make_elem)
    {
        LOG("make test case '{}'", name);
        tests.push_back(make_test_set(name, count, 2, make_elem));
        tests.push_back(make_test_set(name, count, 3, make_elem));
    };

    auto max_cnt = 200'000;
    make_test_set_23("C", max_cnt, make_char);
    make_test_set_23("I", max_cnt, make_rand_i32);
    make_test_set_23("U", max_cnt, make_rand_u32);
    make_test_set_23("L", max_cnt, make_rand_i64);
    make_test_set_23("UL", max_cnt, make_rand_u64);
    make_test_set_23("F", max_cnt, make_rand_float);
    make_test_set_23("D", max_cnt, make_rand_double);
    make_test_set_23("snI", max_cnt, make_small_nat32);
    make_test_set_23("siI", max_cnt, make_small_int32);
    make_test_set_23("snL", max_cnt, make_small_nat64);
    make_test_set_23("siL", max_cnt, make_small_int64);
    make_test_set_23("snF", max_cnt, make_small_nat_float);
    make_test_set_23("siF", max_cnt, make_small_int_float);
    make_test_set_23("snD", max_cnt, make_small_nat_double);
    make_test_set_23("siD", max_cnt, make_small_int_double);
    make_test_set_23("01F", max_cnt, make_float_01);
    make_test_set_23("m11F", max_cnt, make_float_m11);
    make_test_set_23("01D", max_cnt, make_double_01);
    make_test_set_23("m11D", max_cnt, make_double_m11);

    for (auto arity : {2, 3})
    {
        cc::map<tg::ipos2, cc::string> cells;
        auto row = 0;
        {
            cells[{row, 0}] = "";
            auto c = 1;
            for (auto const& t : tests)
            {
                if (t.arity != arity)
                    continue;
                cells[{row, c++}] = t.name;
            }
            ++row;
        }
        auto test_hash_fun = [&](cc::string name, auto hash)
        {
            // LOG("test hash fun '{}'", name);
            cells[{row, 0}] = name + ":";
            auto c = 1;
            for (auto const& t : tests)
            {
                if (t.arity != arity)
                    continue;

                // std::set<std::tuple<size_t, size_t,size_t>> inputs;
                std::set<size_t> hashes;
                int cnt = 0;
                for (auto i = 0; i < int(t.hashes.size()); i += t.arity)
                {
                    auto h0 = t.hashes[i + 0];
                    auto h1 = arity >= 2 ? t.hashes[i + 1] : 0;
                    auto h2 = arity >= 3 ? t.hashes[i + 2] : 0;

                    // CC_ASSERT(!inputs.count({h0, h1}) && "duplicate value");
                    // inputs.insert({h0, h1});

                    auto h = arity == 2 ? hash(h0, h1) : hash(h0, h1, h2);
                    hashes.insert(h);
                    ++cnt;
                }

                auto coll = float(cnt) / hashes.size() - 1;

                cells[{row, c++}] = cc::format("{:.1f}", coll);
            }
            ++row;
        };

        //
        // test funs
        //
#define TEST_HASH_FUN(name) test_hash_fun(#name, [](auto... h) { return hash_combine_##name(h...); })

        TEST_HASH_FUN(simplexor);
        TEST_HASH_FUN(cc);
        TEST_HASH_FUN(boost);
        TEST_HASH_FUN(xxHash);

        //
        // make table
        //
        cc::map<int, int> col_widths;
        auto rows = 0;
        auto cols = 0;
        for (auto const& [p, n] : cells)
        {
            col_widths[p.y] = tg::max(col_widths.get_or(p.y, 4), int(n.size()));
            cols = tg::max(cols, p.y + 1);
            rows = tg::max(rows, p.x + 1);
        }

        std::cout << std::endl;
        std::cout << "vec" << arity << std::endl;
        for (auto r = 0; r < rows; ++r)
        {
            cc::string s;
            [[maybe_unused]] auto align_left = [](cc::string s, int w)
            {
                while (int(s.size()) < w)
                    s += ' ';
                return s;
            };
            [[maybe_unused]] auto align_right = [](cc::string s, int w)
            {
                while (int(s.size()) < w)
                    s.insert(0, " ");
                return s;
            };
            for (auto c = 0; c < cols; ++c)
            {
                s += align_right(cells.get_or({r, c}, ""), col_widths[c]);
                s += "  ";
            }
            std::cout << s.c_str() << std::endl;
        }
    }
}
