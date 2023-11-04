#include <nexus/test.hh>

#include <clean-core/result.hh>
#include <clean-core/string.hh>
#include <clean-core/unique_ptr.hh>

TEST("cc::result basics")
{
    using result_t = cc::result<cc::unique_ptr<int>, cc::string>;

    result_t res;
    CHECK(res.is_error());
    CHECK(!res.is_value());
    CHECK(res.is_error(""));
    CHECK(!res.is_error("error"));
    CHECK(res.error() == "");

    res = cc::string("error");
    CHECK(res.is_error());
    CHECK(!res.is_value());
    CHECK(!res.is_error(""));
    CHECK(res.is_error("error"));
    CHECK(res.error() == "error");

    res = cc::make_unique<int>(17);
    CHECK(!res.is_error());
    CHECK(res.is_value());
    CHECK(*res.value() == 17);
}
