#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/pair.hh>
#include <clean-core/tuple.hh>

TEST("structured bindings")
{
    {
        cc::array<int, 3> a = {3, 5, 7};
        auto [i0, i1, i2] = a;

        CHECK(i0 == 3);
        CHECK(i1 == 5);
        CHECK(i2 == 7);
    }

    {
        cc::tuple<int, float, char> t;

        t.get<0>() = 1;
        t.get<1>() = 3.25f;
        t.get<2>() = 'c';

        auto [i0, i1, i2] = t;

        CHECK(i0 == 1);
        CHECK(i1 == 3.25f);
        CHECK(i2 == 'c');
    }

    {
        cc::pair<int, bool> p = {3, true};

        auto [a, b] = p;

        CHECK(a == 3);
        CHECK(b == true);
    }
}
