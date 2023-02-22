#include <nexus/monte_carlo_test.hh>

#if HAS_CLEAN_RANGES
#include <clean-ranges/algorithms.hh>
#endif

#include <clean-core/experimental/chunked_buffer.hh>
#include <clean-core/vector.hh>

#if HAS_CLEAN_RANGES
TEST("cc::chunked_buffer basics")
{
    cc::chunked_buffer<int> b;
    b.set_chunk_size(100);

    CHECK(b.size() == 0);
    CHECK(cr::count(b.chunks()) == 0);

    for ([[maybe_unused]] auto c : b.chunks())
        CHECK(false); // should not be capped

    b.push_back(1);
    CHECK(b.size() == 1);
    CHECK(cr::count(b.chunks()) == 1);
    for (auto c : b.chunks())
        CHECK(c.size() == 1);

    for (auto i = 0; i < 10; ++i)
        b.push_back(i);
    CHECK(b.size() == 11);
    CHECK(cr::count(b.chunks()) == 1);
    for (auto c : b.chunks())
        CHECK(c.size() == 11);

    for (auto i = 0; i < 100; ++i)
        b.push_back(i);
    CHECK(b.size() == 111);
    CHECK(cr::count(b.chunks()) == 2);
    CHECK(cr::to<cc::vector>(b.chunks(), [](auto c) { return c.size(); }) == nx::range<size_t>{100, 11});
}
#endif

MONTE_CARLO_TEST("cc::chunked_buffer mct")
{
    addOp("gen", [](tg::rng& rng) { return uniform(rng, -10, 10) * 2; });

    addOp("ctor", [] { return cc::chunked_buffer<int>(); });
    addOp("ctor", [] { return cc::vector<int>(); });
    addOp("size", [](cc::chunked_buffer<int> const& buffer) { return buffer.size(); });
    addOp("size", [](cc::vector<int> const& buffer) { return buffer.size(); });
    addOp("push_back", [](cc::chunked_buffer<int>& buffer, int v) { buffer.push_back(v); });
    addOp("push_back", [](cc::vector<int>& buffer, int v) { buffer.push_back(v); });
    addOp("push_back_many",
          [](cc::chunked_buffer<int>& buffer, int v)
          {
              for (auto i = 0; i < 2345; ++i)
                  buffer.push_back(v + i);
          });
    addOp("push_back_many",
          [](cc::vector<int>& buffer, int v)
          {
              for (auto i = 0; i < 2345; ++i)
                  buffer.push_back(v + i);
          });

    testEquivalence(
        [](cc::vector<int> const& a, cc::chunked_buffer<int> const& b)
        {
            cc::vector<int> rhs;
            cc::vector<int> rhs2;

            b.for_each_chunk([&](auto chunk) { rhs.push_back_range(chunk); });

            for (auto c : b.chunks())
                rhs2.push_back_range(c);

            REQUIRE(a == rhs);
            REQUIRE(a == rhs2);
        });
}
