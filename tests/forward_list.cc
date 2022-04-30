#include <nexus/test.hh>

#include <clean-core/any_of.hh>
#include <clean-core/forward_list.hh>

TEST("cc::forward_list")
{
    cc::forward_list<int> l;

    CHECK(l.empty());
    CHECK(l.size() == 0);

    l.emplace_front(3);

    CHECK(!l.empty());
    CHECK(l.size() == 1);

    l.emplace_front(1);
    l.emplace_front(1);

    CHECK(!l.empty());
    CHECK(l.size() == 3);

    for (auto i : l)
        CHECK(i == cc::any_of(1, 3));

    auto l2 = l;

    CHECK(l2.size() == 3);
    for (auto i : l2)
        CHECK(i == cc::any_of(1, 3));
}
