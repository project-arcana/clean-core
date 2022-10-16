#include <nexus/monte_carlo_test.hh>

#include <clean-core/experimental/chunked_buffer.hh>
#include <clean-core/vector.hh>

MONTE_CARLO_TEST("cc::chunked_buffer")
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
            b.for_each_chunk([&](auto chunk) { rhs.push_back_range(chunk); });
            REQUIRE(a == rhs);
        });
}
