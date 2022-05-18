#include <nexus/fuzz_test.hh>

#include <rich-log/log.hh>

#include <clean-core/array.hh>
#include <clean-core/sort.hh>
#include <clean-core/vector.hh>

TEST("cc::sort basics")
{
    cc::vector<int> v = {4, 2, 3, 1};

    cc::sort(v);
    CHECK(v == cc::vector{1, 2, 3, 4});

    cc::sort_by(v, [](auto i) { return -i; });
    CHECK(v == cc::vector{4, 3, 2, 1});
}

TEST("cc::sort api")
{
    // container types
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort(v);
        CHECK(v == cc::vector{1, 2, 3, 4});
    }
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort(cc::span(v));
        CHECK(v == cc::vector{1, 2, 3, 4});
    }
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort(cc::span(v).subspan(1));
        CHECK(v == cc::vector{4, 1, 2, 3});
    }
    {
        cc::array<int> v = {4, 2, 3, 1};
        cc::sort(v);
        CHECK(v == cc::vector{1, 2, 3, 4});
    }
    {
        cc::array<int, 4> v = {4, 2, 3, 1};
        cc::sort(v);
        CHECK(v == cc::vector{1, 2, 3, 4});
    }
    {
        int v[] = {4, 2, 3, 1};
        cc::sort(v);
        CHECK(cc::vector<int>(v) == cc::vector{1, 2, 3, 4});
    }
}

FUZZ_TEST("cc::sort fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 0, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    cc::sort(v);

    for (auto i = 1; i < cnt; ++i)
        CHECK(v[i - 1] <= v[i]);
}

FUZZ_TEST("cc::nth_element fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 1, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    auto idx = uniform(rng, 0, cnt - 1);
    cc::nth_element(v, idx);

    auto nth = v[idx];

    cc::sort(v);

    CHECK(v[idx] == nth);
}

FUZZ_TEST("cc::sort_subrange fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 1, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    auto idx = uniform(rng, 0, cnt - 1);
    auto count = uniform(rng, 1, cnt - idx);

    cc::sort_subrange(v, idx, count);

    auto subrange = cc::vector<int>(cc::span(v).subspan(idx, count));

    cc::sort(v);

    auto ref = cc::vector<int>(cc::span(v).subspan(idx, count));

    CHECK(subrange == ref);
}
