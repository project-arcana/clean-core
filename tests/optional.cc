#include <nexus/test.hh>

#include <clean-core/optional.hh>
#include <clean-core/string.hh>

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
