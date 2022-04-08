#include <nexus/test.hh>

#include <rich-log/log.hh>

#include <clean-core/any_of.hh>
#include <clean-core/flags.hh>

namespace
{
enum class F
{
    a,
    b,
    c
};
CC_FLAGS_ENUM(F);

namespace E
{
enum _t
{
    a,
    b,
    c
};
CC_FLAGS_ENUM(_t);

[[maybe_unused]] cc::string to_string(E::_t f)
{
    switch (f)
    {
    case E::a:
        return "a";
    case E::b:
        return "b";
    case E::c:
        return "c";
    default:
        return "invalid";
    }
}
}

[[maybe_unused]] cc::string to_string(F f)
{
    switch (f)
    {
    case F::a:
        return "a";
    case F::b:
        return "b";
    case F::c:
        return "c";
    default:
        return "invalid";
    }
}
}

TEST("cc::flags enum class")
{
    cc::flags<F> f;
    CHECK(!f.has_any());
    CHECK(!f.has(F::b));

    f = F::c;
    CHECK(f.has_any());
    CHECK(f == F::c);
    CHECK(f != F::b);

    f = {F::a, F::c};
    CHECK((f & F::a));
    CHECK(!(f & F::b));
    CHECK((f & F::c));

    auto f2 = cc::flags(F::a);
    f2 = cc::flags(F::b, f);
    CHECK(f2 == cc::make_flags(F::a, F::b, F::c));

    CHECK(to_string(f2) == "{a, b, c}");

    f = F::b;
    f2 = {F::a, F::b};
    CHECK(f.has_any_of(f2));
    CHECK(!f.has_all_of(f2));

    CHECK(f.is_single());
    CHECK(f.single() == F::b);
    CHECK(!f2.is_single());

    auto f3 = F::c | F::b;
    CHECK((f3 & f2) == F::b);

    for (auto e : f)
        CHECK(e == F::b);
    for (auto e : f2)
        CHECK(e == cc::any_of(F::a, F::b));
    for (auto e : f3)
        CHECK(e == cc::any_of(F::b, F::c));

    auto cnt = [](auto&& f) {
        auto c = 0;
        for (auto e : f)
        {
            ++c;
            (void)e;
        }
        return c;
    };

    CHECK(cnt(f) == 1);
    CHECK(cnt(f2) == 2);
    CHECK(cnt(f2) == 2);

    f2 = cc::no_flags;
    CHECK(cnt(f2) == 0);
}

TEST("cc::flags enum")
{
    cc::flags<E::_t> f;
    CHECK(!f.has_any());
    CHECK(!f.has(E::b));

    f = E::c;
    CHECK(f.has_any());
    CHECK(f == E::c);
    CHECK(f != E::b);

    f = {E::a, E::c};
    CHECK((f & E::a));
    CHECK(!(f & E::b));
    CHECK((f & E::c));

    auto f2 = cc::flags(E::a);
    f2 = cc::flags(E::b, f);
    CHECK(f2 == cc::make_flags(E::a, E::b, E::c));

    CHECK(to_string(f2) == "{a, b, c}");

    f = E::b;
    f2 = {E::a, E::b};
    CHECK(f.has_any_of(f2));
    CHECK(!f.has_all_of(f2));

    CHECK(f.is_single());
    CHECK(f.single() == E::b);
    CHECK(!f2.is_single());

    auto f3 = cc::make_flags(E::c, E::b);
    CHECK((f3 & f2) == E::b);

    for (auto e : f)
        CHECK(e == E::b);
    for (auto e : f2)
        CHECK(e == cc::any_of(E::a, E::b));
    for (auto e : f3)
        CHECK(e == cc::any_of(E::b, E::c));

    auto cnt = [](auto&& f) {
        auto c = 0;
        for (auto e : f)
        {
            ++c;
            (void)e;
        }
        return c;
    };

    CHECK(cnt(f) == 1);
    CHECK(cnt(f2) == 2);
    CHECK(cnt(f3) == 2);

    f2 = cc::no_flags;
    CHECK(cnt(f2) == 0);
}
