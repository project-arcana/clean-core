#include <cstdint>

#include <nexus/monte_carlo_test.hh>

#include <clean-core/array.hh>
#include <clean-core/bit_cast.hh>
#include <clean-core/to_string.hh>

#include <string>
#include <string_view>

TEST("cc::to_string basics")
{
    cc::string s = "234";

    CHECK(cc::to_string(12345) == "12345");
    CHECK(cc::to_string(12345LL) == "12345");
    CHECK(cc::to_string(12345u) == "12345");
    CHECK(cc::to_string(12345uLL) == "12345");
    CHECK(cc::to_string("123") == "123");
    CHECK(cc::to_string(s) == "234");
    CHECK(cc::to_string(true) == "true");
    CHECK(cc::to_string(false) == "false");
    CHECK(cc::to_string('z') == "z");
    CHECK(cc::to_string(nullptr) == "[nullptr]");
    CHECK(cc::to_string((void*)0x1234) == "0x0000000000001234");
    CHECK(cc::to_string(std::byte(1)) == "01");
    CHECK(cc::to_string(std::byte(255)) == "FF");
}

TEST("cc::to_string std")
{
    CHECK(cc::to_string(std::string("hello")) == "hello");
    CHECK(cc::to_string(std::string_view("hello")) == "hello");
}

TEST("cc::to_string pointers")
{
    {
        int* p = nullptr;
        CHECK(cc::to_string(p) == "[nullptr]");
    }
    {
        int* p = (int*)0x1234;
        CHECK(cc::to_string(p) == "0x0000000000001234");
    }
    {
        int const* p = nullptr;
        CHECK(cc::to_string(p) == "[nullptr]");
    }
    {
        char const* p = nullptr;
        CHECK(cc::to_string(p) == "[nullptr]");
    }
    {
        char const* p = "hello";
        CHECK(cc::to_string(p) == "hello");
    }
    {
        char s[] = {'A', 'B', 'C', 0};
        char* p = s;
        CHECK(cc::to_string(p) == "ABC");
    }
}

namespace
{
template <class T>
T gen_random(tg::rng& rng)
{
    cc::array<tg::u32, sizeof(T) / sizeof(tg::u32)> v;
    for (auto& i : v)
        i = rng();
    return cc::bit_cast<T>(v);
}
}

MONTE_CARLO_TEST("cc::to_string mct")
{
    addOp("gen", gen_random<int32_t>);
    addOp("gen", gen_random<int64_t>);
    addOp("gen", gen_random<uint32_t>);
    addOp("gen", gen_random<uint64_t>);
    addOp("gen", gen_random<float>);
    addOp("gen", gen_random<double>);
    addOp("gen", gen_random<void*>);

    addValue("+inf", tg::inf<float>);
    addValue("+inf", tg::inf<double>);
    addValue("-inf", -tg::inf<float>);
    addValue("-inf", -tg::inf<double>);
    addValue("nan", tg::nan<float>);
    addValue("nan", tg::nan<double>);

    addOp("to_string", (cc::string(*)(int32_t))cc::to_string);
    addOp("to_string", (cc::string(*)(int64_t))cc::to_string);
    addOp("to_string", (cc::string(*)(uint32_t))cc::to_string);
    addOp("to_string", (cc::string(*)(uint64_t))cc::to_string);
    addOp("to_string", (cc::string(*)(float))cc::to_string);
    addOp("to_string", (cc::string(*)(double))cc::to_string);
    addOp("to_string", (cc::string(*)(void*))cc::to_string);

    addOp("round-trip", [](int i) { CHECK(i == std::stoi(cc::to_string(i).c_str())); });

    addInvariant("non-empty", [](cc::string const& s) { CHECK(!s.empty()); });
}
