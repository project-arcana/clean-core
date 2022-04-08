#include <nexus/test.hh>

#include <clean-core/function_ref.hh>

namespace
{
int test_fun(int a, int b) { return a + b; }

struct test_callable
{
    int x = 0;
    int operator()(int a, int b) const { return a * b + x; }
};
}

TEST("cc::function_ref")
{
    cc::function_ref<int(int, int)> f = test_fun;

    CHECK(f(1, 2) == 3);

    f = &test_fun;
    CHECK(f(1, 2) == 3);

    int x = 7;
    auto l = [&](int a, int b) { return a + b + x; };

    f = l;
    CHECK(f(1, 2) == 10);

    x = 5;
    CHECK(f(1, 2) == 8);

    f = [](int a, int b) { return a * b; }; // careful, lambda lifetime
    CHECK(f(3, 4) == 12);

    test_callable tc;
    f = tc;
    CHECK(f(2, 3) == 6);
    tc.x = 10;
    CHECK(f(2, 3) == 16);

    test_callable const ctc = {9};
    f = ctc;
    CHECK(f(2, 3) == 15);
}
