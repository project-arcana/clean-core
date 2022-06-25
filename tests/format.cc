#include <nexus/test.hh>

#include <clean-core/format.hh>

TEST("cc::format basics")
{
    CHECK(cc::format("{}", 17) == "17");
    CHECK(cc::format("{} + {} = {}", 1, 2, 3) == "1 + 2 = 3");

    // reordering
    CHECK(cc::format("{2} - {0} = {1}", 1, 2, 3) == "3 - 1 = 2");
    CHECK(cc::format("{1} {0}!", "World", "Hello") == "Hello World!");

    // escaping
    CHECK(cc::format("this {{}} is used for args like {}", "this") == "this {} is used for args like this");

    // format strings
    CHECK(cc::format("{:4}", 12) == "  12");
    CHECK(cc::format("{:.2f}", 1.2345) == "1.23");

    // decorators?
    // cc::format("{}", cc::fmt_join(my_vec, ", ")))
}

TEST("cc::format opinionated")
{
    CHECK(cc::formatf("{}") == "{}");
    CHECK(cc::formatp("%s") == "%s");
}
