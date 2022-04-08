#include <nexus/fuzz_test.hh>

#include <climits>

#include <clean-core/utility.hh>

FUZZ_TEST("min/max/clamp fuzz")(tg::rng& rng)
{
    int const num_a = tg::uniform(rng, INT_MIN, INT_MAX);
    int num_b;

    do
        num_b = tg::uniform(rng, INT_MIN, INT_MAX);
    while (num_a == num_b);

    REQUIRE(num_a != num_b);

    int const num_lo = num_a > num_b ? num_b : num_a;
    int const num_hi = num_a > num_b ? num_a : num_b;
    REQUIRE(cc::min(num_a, num_b) == num_lo);
    REQUIRE(cc::max(num_a, num_b) == num_hi);

    CHECK(cc::clamp(num_lo, num_lo, num_hi) == num_lo);
    CHECK(cc::clamp(num_hi, num_lo, num_hi) == num_hi);

    CHECK(cc::clamp(num_lo + 1, num_lo, num_hi) == num_lo + 1);
    CHECK(cc::clamp(num_hi - 1, num_lo, num_hi) == num_hi - 1);
}

TEST("utility")
{
    CHECK(cc::wrapped_increment(0, 1) == 0);
    CHECK(cc::wrapped_increment(0, 5) == 1);
    CHECK(cc::wrapped_increment(4, 5) == 0);

    CHECK(cc::wrapped_decrement(0, 5) == 4);
    CHECK(cc::wrapped_decrement(4, 5) == 3);

    CHECK(cc::int_div_ceil(1, 1) == 1);
    CHECK(cc::int_div_ceil(6, 3) == 2);
    CHECK(cc::int_div_ceil(7, 3) == 3);
    CHECK(cc::int_div_ceil(8, 3) == 3);
    CHECK(cc::int_div_ceil(9, 3) == 3);
    CHECK(cc::int_div_ceil(10, 3) == 4);
}
