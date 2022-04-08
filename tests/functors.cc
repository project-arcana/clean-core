#include <nexus/test.hh>

#include <clean-core/functors.hh>
#include <clean-core/vector.hh>

TEST("void functor")
{
    auto test = [](auto f)
    {
        f();
        f(1);
        f(1, true);
        f(1, true, false, "hi", cc::vector<int>{});
    };
    test(cc::void_function{});

    CHECK(true); // only checks if compiles
}

TEST("id functor")
{
    auto test = [](auto f)
    {
        CHECK(f(1) == 1);
        CHECK(f(true) == true);
        CHECK(f(cc::vector<int>{1, 2, 3}) == cc::vector<int>{1, 2, 3});
    };

    test(cc::identity_function{});
}

TEST("constant functor")
{
    auto test = [](auto f)
    {
        CHECK(f() == 17);
        CHECK(f(1) == 17);
        CHECK(f(1, true) == 17);
        CHECK(f(1, true, false, "hi", cc::vector<int>{}) == 17);
    };
    test(cc::constant_function<17>{});
}

TEST("projection functor")
{
    auto test = [](auto f0, auto f1, auto f2)
    {
        CHECK(f0(1) == 1);
        CHECK(f0(true, 1) == true);
        CHECK(f0(cc::vector<int>{1, 2, 3}, 1, true, false, "hi", cc::vector<int>{}) == cc::vector<int>{1, 2, 3});

        CHECK(f1(true, 1) == 1);
        CHECK(f1(cc::vector<int>{1, 2, 3}, false, "hi", cc::vector<int>{}) == false);

        CHECK(f2(true, 1, 'c') == 'c');
        CHECK(f2(1, 2, cc::vector<int>{1, 2, 3}, false, cc::vector<int>{}) == cc::vector<int>{1, 2, 3});
    };
    test(cc::projection_function<0>{}, cc::projection_function<1>{}, cc::projection_function<2>{});
}
