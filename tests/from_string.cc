
#include <cstdint>

#include <nexus/fuzz_test.hh>
#include <nexus/test.hh>

#include <clean-core/from_string.hh>

TEST("cc::from_string")
{
    int32_t v;
    CHECK(cc::from_string("123", v));
    CHECK(v == 123);
    CHECK(!cc::from_string("123 trailing text", v));
}

namespace
{
template <class T>
void check_range(tg::rng& rng, T min, T max)
{
    T v0 = min;
    T v1 = min;
    for (auto i = 0; i < 100; ++i)
    {
        v0 = uniform(rng, min, max);
        auto s = cc::to_string(v0);
        auto ok = cc::from_string(s, v1);
        CHECK(ok);
        CHECK(v0 == v1);
    }
};

template <class T>
void check_range_near(tg::rng& rng, T min, T max, T tol)
{
    T v0 = min;
    T v1 = min;
    for (auto i = 0; i < 100; ++i)
    {
        v0 = uniform(rng, min, max);
        auto s = cc::to_string(v0);
        auto ok = cc::from_string(s, v1);
        CHECK(ok);
        CHECK(v0 == nx::approx(v1).abs(tol));
    }
};
} // anon namespace

FUZZ_TEST("cc::from_string fuzz")(tg::rng& rng)
{
    check_range<int32_t>(rng, -100, 100);
    check_range<int64_t>(rng, -100000000000, 100000000000);
    check_range<uint32_t>(rng, 5, 100);
    check_range<uint64_t>(rng, 5, 100);
    check_range_near<double>(rng, -100., 100., 0.01);
    check_range_near<float>(rng, -100.f, 100.f, 0.01f);
}
