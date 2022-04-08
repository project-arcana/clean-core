#include <nexus/test.hh>

#include <clean-core/strided_span.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/range.hh>

TEST("cc::strided_span (span equivalent)")
{
    cc::vector<int> v = {1, 2, 3};

    auto s = cc::strided_span(v);
    CHECK(s.size() == 3);
    CHECK(s[0] == 1);
    CHECK(s[2] == 3);

    s = s.subspan(1, 2);
    CHECK(s.size() == 2);
    CHECK(s[0] == 2);
    CHECK(s[1] == 3);

    int va[] = {3, 2, 5, 6};
    s = cc::strided_span(va);
    CHECK(s.size() == 4);
    CHECK(s[0] == 3);
    CHECK(s[3] == 6);
    s[1] += 2;
    CHECK(va[1] == 4);

    int x = 8;
    s = cc::strided_span(x);
    CHECK(s.size() == 1);
    CHECK(s[0] == 8);
    x = 9;
    CHECK(s[0] == 9);

    s = {v.data(), v.size()};
    CHECK(s.size() == 3);
    CHECK(s[0] == 1);
    CHECK(s[2] == 3);

    s = s.subspan(2);
    CHECK(s.size() == 1);
    CHECK(s[0] == 3);

    s = s.subspan(1);
    CHECK(s.size() == 0);
    CHECK(s.empty());

    s = v;
    CHECK(s.size() == 3);
    CHECK(cr::range(s) == cc::vector{1, 2, 3});
    CHECK(cr::range(s.reversed()) == cc::vector{3, 2, 1});

    s = s.first(2);
    CHECK(cr::range(s) == cc::vector{1, 2});

    s = cc::strided_span(v).last(2);
    CHECK(cr::range(s) == cc::vector{2, 3});
}
