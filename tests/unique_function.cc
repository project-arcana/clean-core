#include <nexus/test.hh>

#include <clean-core/unique_function.hh>

namespace
{
struct callable_t
{
    void operator()() {}
};

void void_func() {}
}

TEST("cc::unique_function")
{
    cc::unique_function<int(int)> f;

    CHECK(!f);

    f = [](int i) { return i * 2; };

    CHECK(f(7) == 14);

    auto f2 = cc::move(f);

    CHECK(!f);
    CHECK(f2(8) == 16);
}

TEST("cc::unique_function compilation", disabled)
{
    auto lambda = []() -> void {};
    cc::function_ptr<void()> ptr = +[]() -> void {};
    callable_t type;

    cc::unique_function<void()> f_tl = type;
    cc::unique_function<void()> f_tr = callable_t{};
    cc::unique_function<void()> f_ll = lambda;
    cc::unique_function<void()> f_lr = [type]() -> void { (void)type; };
    cc::unique_function<void()> f_ptrl = ptr;
    cc::unique_function<void()> f_ptrr = +[]() -> void {};
    cc::unique_function<void()> f_ptrf = void_func;

    CHECK(true);
}
