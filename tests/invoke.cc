#include <nexus/test.hh>

#include <clean-core/invoke.hh>

namespace
{
int plus_two(int x) { return x + 2; }
int& ref_id(int& x) { return x; }
}

TEST("cc::invoke")
{
    int x = 10;
    auto lambda_f = [&](int a) { return a + x; };

    struct foo
    {
        int v;
        int& get_v() { return v; }
        int bar() { return v + 1; }
    };

    CHECK(cc::invoke(lambda_f, 7) == 17);
    CHECK(cc::invoke(plus_two, 7) == 9);
    CHECK(cc::invoke(ref_id, x) == 10);
    cc::invoke(ref_id, x) = 4;
    CHECK(x == 4);

    foo f;
    f.v = 9;
    CHECK(cc::invoke(&foo::v, f) == 9);

    cc::invoke(&foo::v, f) = 5;
    CHECK(f.v == 5);

    CHECK(cc::invoke(&foo::bar, f) == 6);
    CHECK(cc::invoke(&foo::get_v, f) == 5);

    cc::invoke(&foo::get_v, f) = 11;
    CHECK(f.v == 11);
}
