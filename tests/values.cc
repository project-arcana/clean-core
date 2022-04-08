#include <nexus/monte_carlo_test.hh>

#include <clean-core/box.hh>
#include <clean-core/capped_box.hh>
#include <clean-core/fwd_box.hh>
#include <clean-core/poly_box.hh>
#include <clean-core/poly_unique_ptr.hh>
#include <clean-core/polymorphic.hh>
#include <clean-core/unique_ptr.hh>

TEST("cc::box")
{
    auto b = cc::box(1);
    CHECK(*b == 1);

    b = 7;
    CHECK(b == 7);

    *b = 9;
    CHECK(b == 9);

    b = cc::make_box<int>(17);
    CHECK(b == 17);

    // comparisons
    b = 10;
    auto c = cc::make_box<int>(17);
    CHECK(b == 10);
    CHECK(b != c);
    CHECK(10 == b);
    CHECK(b > 5);
    CHECK(b < 15);
    CHECK(b >= 5);
    CHECK(b <= 15);
    CHECK(b != 7);
    CHECK(5 < b);
    CHECK(15 > b);
    CHECK(5 <= b);
    CHECK(15 >= b);
    CHECK(7 != b);

    int i = b;
    CHECK(i == 10);

    b = cc::box(i);
    b = i;
}

namespace
{
struct fwd_decl;
}

TEST("cc::fwd_box")
{
    auto b = cc::make_fwd_box<int>(8);

    CHECK(b == 8);

    *b = 9;
    CHECK(*b == 9);

    // make sure incomplete types work
    auto f = [](cc::fwd_box<fwd_decl>&& b) { return cc::move(b); };
    (void)f;
}

TEST("cc::poly_box")
{
    struct A
    {
        virtual int value() const = 0;
        virtual ~A() = default;
    };
    struct B : A
    {
        int value() const override { return 7; }
    };
    struct C : A
    {
        int value() const override { return 10; }
    };

    cc::poly_box<A> b = cc::make_poly_box<B>();

    CHECK(b->value() == 7);

    b = C();
    CHECK(b->value() == 10);

    b = cc::make_poly_box<B>();
    CHECK(b->value() == 7);

    cc::poly_box<A> b2 = C();
    CHECK(b2->value() == 10);

    b.emplace<C>();
    CHECK(b->value() == 10);
}

TEST("cc::capped_box")
{
    struct A
    {
        virtual int value() const = 0;
        virtual ~A() = default;
    };
    struct B : A
    {
        int value() const override { return 7; }
    };
    struct C : A
    {
        int _pad = -1;
        int _val;
        C(int i) : _val(i) {}
        int value() const override { return _val; }
    };

    cc::capped_box<A, sizeof(C)> b = B();

    CHECK(b->value() == 7);

    b = C(17);
    CHECK(b->value() == 17);

    b = cc::make_capped_box<A, sizeof(C), B>();
    CHECK(b->value() == 7);

    b.emplace<C>(9);
    CHECK(b->value() == 9);
}

MONTE_CARLO_TEST("value mct")
{
    auto const make_int = [](tg::rng& rng) { return uniform(rng, -10, 10); };

    addOp("gen int", make_int);

    auto const addType = [&](auto obj) {
        using value_t = decltype(obj);
        using T = std::decay_t<decltype(*obj)>;

        addOp("make", [](T t) { return value_t(t); });
        addOp("assign", [](value_t& v, T t) { v = t; });
        addOp("move assign", [](value_t& v, T t) { v = value_t(t); });
        addOp("move ctor", [](value_t& v, value_t& r) {
            v = cc::move(r);
            r = -1;
        });
        addOp("get value", [](value_t const& v) { return *v; });
        addOp("get value (impl)", [](value_t const& v) -> T { return v; });

        addOp("<", [](value_t const& a, value_t const& b) { return a < b; });
        addOp("<=", [](value_t const& a, value_t const& b) { return a <= b; });
        addOp(">", [](value_t const& a, value_t const& b) { return a > b; });
        addOp(">=", [](value_t const& a, value_t const& b) { return a >= b; });
        addOp("==", [](value_t const& a, value_t const& b) { return a == b; });
        addOp("!=", [](value_t const& a, value_t const& b) { return a != b; });

        addOp("T <", [](T const& a, value_t const& b) { return a < b; });
        addOp("T <=", [](T const& a, value_t const& b) { return a <= b; });
        addOp("T >", [](T const& a, value_t const& b) { return a > b; });
        addOp("T >=", [](T const& a, value_t const& b) { return a >= b; });
        addOp("T ==", [](T const& a, value_t const& b) { return a == b; });
        addOp("T !=", [](T const& a, value_t const& b) { return a != b; });

        addOp("< T", [](value_t const& a, T const& b) { return a < b; });
        addOp("<= T", [](value_t const& a, T const& b) { return a <= b; });
        addOp("> T", [](value_t const& a, T const& b) { return a > b; });
        addOp(">= T", [](value_t const& a, T const& b) { return a >= b; });
        addOp("== T", [](value_t const& a, T const& b) { return a == b; });
        addOp("!= T", [](value_t const& a, T const& b) { return a != b; });
    };

    // TODO: more
    addType(cc::make_box<int>());
}
