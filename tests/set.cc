#include <nexus/monte_carlo_test.hh>

#include <rich-log/log.hh>

#include <clean-core/any_of.hh>
#include <clean-core/set.hh>

TEST("cc::set")
{
    bool b;
    cc::set<int> s;

    static_assert(cc::is_range<cc::set<int>, int const>);
    static_assert(cc::is_range<cc::set<int>&, int const>);

    CHECK(s.empty());
    CHECK(s.size() == 0);
    CHECK(!s.contains(3));

    s.add(3);
    CHECK(s.size() == 1);
    CHECK(s.contains(3));
    CHECK(!s.contains(4));

    s.add(3);
    CHECK(s.size() == 1);

    s.add(5);
    CHECK(s.size() == 2);

    b = s.remove(7);
    CHECK(!b);
    CHECK(s.size() == 2);

    b = s.remove(3);
    CHECK(b);
    CHECK(s.size() == 1);
    CHECK(!s.contains(3));

    b = s.remove(5);
    CHECK(b);
    CHECK(s.size() == 0);
    CHECK(!s.contains(5));

    s = {1, 2, 3, 2};
    CHECK(s.size() == 3);
    CHECK(s.contains(2));

    auto s2 = s; // copy set
    CHECK(s2.size() == 3);
    CHECK(s2.contains(1));

    auto cnt = 0;
    for (auto i : s)
    {
        ++cnt;
        CHECK(i == cc::any_of(1, 2, 3));
        CHECK(i >= 1);
        CHECK(i <= 3);
    }
    CHECK(cnt == 3);

    s |= 2;
    CHECK(s.size() == 3);

    s |= 5;
    CHECK(s.size() == 4);

    s |= {1, 3, 5, 7};
    CHECK(s.size() == 5);

    s = {1, 3, 5};
    s2 = {5, 1, -3};
    CHECK(s.size() == 3);
    CHECK(s2.size() == 3);

    s = s | s2;
    CHECK(s.size() == 4);
    for (auto i : s)
        CHECK(i == cc::any_of(-3, 1, 3, 5));
}
