#include <nexus/test.hh>

#include <clean-core/any_of.hh>
#include <clean-core/vector.hh>

TEST("cc::any_of")
{
    CHECK(1 == cc::any_of(1, 2, 3));
    CHECK(2 == cc::any_of(1, 2, 3));
    CHECK(3 == cc::any_of(1, 2, 3));

    CHECK(4 != cc::any_of(1, 2, 3));

    CHECK(5 == cc::any_of(1, 2, 3, 4, 5, 6));

    cc::vector<int> v = {1, 2, 3};

    static_assert(cc::is_any_range<cc::vector<int>>);

    CHECK(1 == cc::any_of(v));
    CHECK(2 == cc::any_of(v));
    CHECK(3 == cc::any_of(v));

    CHECK(4 != cc::any_of(v));
}
