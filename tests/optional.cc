#include <nexus/test.hh>

#include <clean-core/optional.hh>
#include <clean-core/string.hh>
#include <clean-core/to_string.hh>

#include <typed-geometry/feature/basic.hh>

TEST("cc::optional basics")
{
    cc::optional<int> v;
    CHECK(!v.has_value());
    CHECK(v != 0);

    v = 7;
    CHECK(v == 7);
    CHECK(v != 8);
    CHECK(v != cc::nullopt);

    auto vv = cc::make_optional(13);
    CHECK(v != vv);
    vv = 7;
    CHECK(v == vv);

    auto vf = cc::make_optional(7.f);
    CHECK(v == vf);
    vf = 8;
    CHECK(v != vf);

    v = vf;
    CHECK(v == 8);

    v = {};
    CHECK(!v.has_value());

    v = 3;
    CHECK(v.has_value());

    v = cc::nullopt;
    CHECK(!v.has_value());
}

TEST("cc::optional string")
{
    cc::optional<cc::string> v;
    CHECK(!v.has_value());

    v = "hello";
    CHECK(v == "hello");

    v = {};
    CHECK(v != "hello");
    CHECK(!v.has_value());
}

TEST("cc::optional map")
{
    cc::optional<int> i = 17;
    CHECK(i == 17);

    i = i.map([](int x) { return -x * 2; });
    CHECK(i == -34);

    auto iabs = [](int x) { return tg::abs(x); };
    i = i.map(iabs);
    CHECK(i == 34);

    auto s = i.map([](int x) { return cc::to_string(x); });
    CHECK(s == "34");

    i = cc::nullopt;
    CHECK(!i.has_value());

    s = i.map([](int x) { return cc::to_string(x); });
    CHECK(!s.has_value());

    i = i.map(iabs);
    CHECK(!i.has_value());

    i = 123;
    s = i.map([](int x) { return cc::to_string(x); });
    CHECK(s == "123");

    i = s.map(&cc::string::size);
    CHECK(i == 3);

    cc::optional<tg::pos3> p = tg::pos3(1, 2, 3);
    i = p.map(&tg::pos3::y);
    CHECK(i == 2);
}

TEST("cc::optional transform")
{
    cc::optional<int> i = 17;
    CHECK(i == 17);

    i.transform([](int& x) { x *= 2; });
    CHECK(i == 34);

    i = cc::nullopt;
    CHECK(!i.has_value());

    i.transform([](int& x) { x *= 2; });
    CHECK(!i.has_value());

    cc::optional<cc::string> s = cc::string("hello");
    CHECK(s == "hello");

    s.transform(&cc::string::clear);
    CHECK(s == "");

    s = cc::nullopt;
    CHECK(!s.has_value());

    s.transform(&cc::string::clear);
    CHECK(!s.has_value());
}
