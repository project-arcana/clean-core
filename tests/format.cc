#include <nexus/test.hh>

#include <clean-core/format.hh>

TEST("cc::format basics")
{
    CHECK(cc::format("{}", 17) == "17");
    CHECK(cc::format("{} + {} = {}", 1, 2, 3) == "1 + 2 = 3");
    CHECK(cc::format("%s + %d = %x", 1, 2, 3) == "1 + 2 = 3");
    CHECK(cc::format("{}{}{}", 1, 2, 3) == "123");
    CHECK(cc::format("%s%d%x", 1, 2, 3) == "123");

    // reordering
    CHECK(cc::format("{2} - {0} = {1}", 1, 2, 3) == "3 - 1 = 2");
    CHECK(cc::format("{1} {0}!", "World", "Hello") == "Hello World!");

    // escaping
    CHECK(cc::format("this {{}} is used for args like {}", "this") == "this {} is used for args like this");
    CHECK(cc::format("look ma, I can write %% and {{ and }}") == "look ma, I can write % and { and }");

    // format strings
    CHECK(cc::format("{:4}", 12) == "  12");
    CHECK(cc::format("%4d", 12) == "  12");
    CHECK(cc::format("{:.2f}", 1.2345) == "1.23");
    CHECK(cc::format("%.2f", 1.2345) == "1.23");

    // decorators?
    // cc::format("{}", cc::fmt_join(my_vec, ", ")))
}

TEST("cc::format opinionated")
{
    CHECK(cc::formatf("{} %s", "x") == "{} x");
    CHECK(cc::formatp("{} %s", "x") == "x %s");
}
