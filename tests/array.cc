#include <nexus/monte_carlo_test.hh>
#include <nexus/test.hh>

#include <array>

#include <clean-core/alloc_array.hh>
#include <clean-core/array.hh>
#include <clean-core/assert.hh>
#include <clean-core/capped_array.hh>
#include <clean-core/fwd_array.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg.hh>

TEST("cc::array")
{
    cc::array<int> a;
    CHECK(a.empty());

    a = {1, 2, 3};
    CHECK(a.size() == 3);

    CHECK(tg::sum(a) == 6);

    cc::array<int> b = a;
    CHECK(a == b);

    b[1] = 7;
    CHECK(a != b);

    b = std::move(a);
    CHECK(a.empty());
    CHECK(tg::sum(b) == 6);
}

TEST("cc::alloc_array")
{
    cc::alloc_array<int> a;
    CHECK(a.empty());

    a = {1, 2, 3};
    CHECK(a.size() == 3);

    CHECK(tg::sum(a) == 6);

    cc::alloc_array<int> b(cc::span<int>{a});
    CHECK(a == b);

    b[1] = 7;
    CHECK(a != b);

    b = std::move(a);
    CHECK(a.empty());
    CHECK(tg::sum(b) == 6);
}

TEST("cc::array fixed")
{
    cc::array<int, 3> a;

    a = cc::make_array(1, 2, 3);
    CHECK(tg::sum(a) == 6);

    // Test if initializer lists create fixed cc::arrays
    {
        cc::array list = {1, 2, 3};
        cc::array list_single = {1};
        cc::array list_single_size_t = {size_t(42)};

        static_assert(std::is_same_v<decltype(list), cc::array<int, 3>>);
        static_assert(std::is_same_v<decltype(list_single), cc::array<int, 1>>);
        static_assert(std::is_same_v<decltype(list_single_size_t), cc::array<size_t, 1>>);
    }
}

MONTE_CARLO_TEST("cc::array + fwd_array mct")
{
    auto const make_int = [](tg::rng& rng) { return uniform(rng, -10, 10); };

    addOp("gen int", make_int);

    auto const addType = [&](auto obj) {
        using array_t = decltype(obj);

        addOp("default ctor", [] { return array_t(); });

        if constexpr (std::is_copy_constructible_v<array_t>)
        {
            addOp("move ctor", [](array_t const& s) { return cc::move(array_t(s)); }).make_optional();
            addOp("move assignment", [](array_t& a, array_t const& b) { a = array_t(b); }).make_optional();

            addOp("copy ctor", [](array_t const& s) { return array_t(s); }).make_optional();
            addOp("copy assignment", [](array_t& a, array_t const& b) { a = b; }).make_optional();
        }

        addOp("defaulted", [](tg::rng& rng) { return array_t::defaulted(uniform(rng, 0, 15)); });
        addOp("filled", [](tg::rng& rng, int v) { return array_t::filled(uniform(rng, 0, 15), v); });

        addOp("randomize", [&](tg::rng& rng, array_t& s) {
            for (auto& v : s)
                v = make_int(rng);
        });

        addOp("random replace", [&](tg::rng& rng, array_t& s) { random_choice(rng, s) = make_int(rng); }).when([](tg::rng&, array_t const& s) {
            return s.size() > 0;
        });

        addOp("op[]", [](tg::rng& rng, array_t const& s) { return random_choice(rng, s); }).when([](tg::rng&, array_t const& s) {
            return s.size() > 0;
        });
        addOp("data[]", [](tg::rng& rng, array_t const& s) {
            return s.data()[uniform(rng, 0, int(s.size()) - 1)];
        }).when([](tg::rng&, array_t const& s) { return s.size() > 0; });

        addOp("fill", [](array_t& s, int v) {
            for (auto& c : s)
                c = v;
        });

        addOp("size", [](array_t const& a) { return a.size(); });
        addOp("empty", [](array_t const& a) { return a.empty(); });
    };

    addType(cc::array<int>());
    addType(cc::vector<int>());
    addType(cc::fwd_array<int>());
    addType(cc::capped_array<int, 25>());

    testEquivalence([](cc::array<int> const& a, cc::vector<int> const& b) {
        REQUIRE(a.size() == b.size());
        for (auto i = 0; i < int(a.size()); ++i)
            REQUIRE(a[i] == b[i]);
    });
    testEquivalence([](cc::array<int> const& a, cc::fwd_array<int> const& b) {
        REQUIRE(a.size() == b.size());
        for (auto i = 0; i < int(a.size()); ++i)
            REQUIRE(a[i] == b[i]);
    });
    testEquivalence([](cc::array<int> const& a, cc::capped_array<int, 25> const& b) {
        REQUIRE(a.size() == b.size());
        for (auto i = 0; i < int(a.size()); ++i)
            REQUIRE(a[i] == b[i]);
    });
}
