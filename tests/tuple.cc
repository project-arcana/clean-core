#include <nexus/test.hh>

#include <clean-core/apply.hh>
#include <clean-core/tuple.hh>

TEST("cc::tuple")
{
    cc::tuple<int, float, char> t;

    t.get<0>() = 1;
    t.get<1>() = 3.25f;
    t.get<2>() = 'c';

    static_assert(sizeof(t) == 3 * sizeof(int));

    CHECK(t.get<0>() == 1);
    CHECK(t.get<1>() == 3.25f);
    CHECK(t.get<2>() == 'c');

    t = {2, 1.5f, 'a'};

    auto [a, b, c] = t;
    CHECK(a == 2);
    CHECK(b == 1.5f);
    CHECK(c == 'a');

    auto t2 = cc::tuple(2, 1.6f, 'a');
    CHECK(t != t2);

    float bf = 10;
    auto lambda_f = [&](int i, float f, char c) {
        bf += 1;
        return int(i + f + c + bf);
    };

    CHECK(cc::apply(lambda_f, t) == 2 + 1 + int('a') + 11);
    CHECK(bf == 11);
}
