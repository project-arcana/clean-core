#include <nexus/fuzz_test.hh>

#include <rich-log/log.hh>

#include <clean-core/array.hh>
#include <clean-core/sort.hh>
#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

#include <reflector/compare.hh>

#include <typed-geometry/tg-lean.hh>

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
        CHECK(cc::vector<int>(v) == cc::vector{1, 2, 3, 4});
    }
    {
        cc::array<int, 4> v = {4, 2, 3, 1};
        cc::sort(v);
        CHECK(cc::vector<int>(v) == cc::vector{1, 2, 3, 4});
    }
    {
        int v[] = {4, 2, 3, 1};
        cc::sort(v);
        CHECK(cc::vector<int>(v) == cc::vector{1, 2, 3, 4});
    }

    // comparators
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort(v, [](auto a, auto b) { return a > b; });
        CHECK(v == cc::vector{4, 3, 2, 1});
    }
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort(v, rf::greater{});
        CHECK(v == cc::vector{4, 3, 2, 1});
    }

    // sort_by
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort_by(v, [](auto i) { return -i; });
        CHECK(v == cc::vector{4, 3, 2, 1});
    }
    {
        cc::vector<tg::vec3> v = {{1, 12, 3}, {6, 5, 2}, {-1, 6, 10}};
        cc::sort_by(v, &tg::vec3::y);
        CHECK(v == cc::vector<tg::vec3>{{6, 5, 2}, {-1, 6, 10}, {1, 12, 3}});
    }
    {
        cc::vector<tg::vec3> v = {{1, 12, 3}, {6, 5, 2}, {-1, 6, 10}};
        cc::sort_by(v, [](tg::vec3 const& v) { return v.z; });
        CHECK(v == cc::vector<tg::vec3>{{6, 5, 2}, {1, 12, 3}, {-1, 6, 10}});
    }

    // descending
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort_descending(v);
        CHECK(v == cc::vector{4, 3, 2, 1});
    }
    {
        cc::vector<int> v = {4, 2, 3, 1};
        cc::sort_by_descending(v, [](int i) { return -i; });
        CHECK(v == cc::vector{1, 2, 3, 4});
    }

    // multi sort
    {
        cc::vector<int> k = {4, 2, 3, 1};
        cc::vector<char> v = {'A', 'B', 'C', 'D'};
        cc::sort_multi(cc::less<>{}, k, v);
        CHECK(k == cc::vector{1, 2, 3, 4});
        CHECK(v == cc::vector{'D', 'B', 'C', 'A'});
    }
    {
        cc::vector<int> k = {4, 2, 3, 1};
        cc::vector<char> v = {'A', 'B', 'C', 'D'};
        cc::sort_multi_by([](int i, char) { return i; }, cc::less<>{}, k, v);
        CHECK(k == cc::vector{1, 2, 3, 4});
        CHECK(v == cc::vector{'D', 'B', 'C', 'A'});
    }

    // move-only stuff
    {
        cc::vector<cc::unique_ptr<int>> v;
        v.push_back(cc::make_unique<int>(4));
        v.push_back(cc::make_unique<int>(2));
        v.push_back(cc::make_unique<int>(3));
        v.push_back(cc::make_unique<int>(1));
        cc::sort_by(
            v, [o = cc::make_unique<int>(10)](auto const& p) { return -*p + *o; },
            [o = cc::make_unique<int>(10)](int a, int b) { return a + *o > b + *o; });
        CHECK(*v[0] == 1);
        CHECK(*v[1] == 2);
        CHECK(*v[2] == 3);
        CHECK(*v[3] == 4);
    }
}

FUZZ_TEST("cc::sort fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 0, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    cc::sort(v);

    CHECK(cc::is_sorted(v));

    for (auto i = 1; i < cnt; ++i)
        CHECK(v[i - 1] <= v[i]);

    if (cc::is_strictly_sorted(v))
        for (auto i = 1; i < cnt; ++i)
            CHECK(v[i - 1] < v[i]);
}

FUZZ_TEST("cc::partition fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 0, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    auto ref = uniform(rng, -100, 100);

    auto idx = cc::partition_by(v, [&](int i) { return i >= ref; });

    for (auto i = 0; i < idx; ++i)
        CHECK(v[i] < ref);
    for (auto i = idx; i < cnt; ++i)
        CHECK(v[i] >= ref);
}

FUZZ_TEST("cc::quickselect fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 1, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    auto idx = uniform(rng, 0, cnt - 1);
    cc::quickselect(v, idx);

    auto nth = v[idx];

    cc::sort(v);

    CHECK(v[idx] == nth);
}

FUZZ_TEST("cc::quickselect_range fuzzer")(tg::rng& rng)
{
    cc::vector<int> v;

    auto cnt = uniform(rng, 1, 200);
    for (auto i = 0; i < cnt; ++i)
        v.push_back(uniform(rng, -100, 100));

    auto idx = uniform(rng, 0, cnt - 1);
    auto count = uniform(rng, 1, cnt - idx);

    cc::quickselect_range(v, idx, count);

    auto subrange = cc::vector<int>(cc::span(v).subspan(idx, count));
    CHECK(cc::is_sorted(subrange));

    cc::sort(v);

    auto ref = cc::vector<int>(cc::span(v).subspan(idx, count));

    CHECK(subrange == ref);
}
