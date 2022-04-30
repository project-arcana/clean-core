#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/range_ref.hh>
#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

TEST("cc::range_ref")
{
    auto check_range = [](cc::range_ref<int> r) {
        cc::vector<int> v;
        r.for_each([&](int i) { v.push_back(i); });
        CHECK(v == cc::vector<int>{1, 2, 3});
    };

    {
        cc::vector<int> v = {1, 2, 3};
        check_range(v);
        check_range(cc::make_range_ref(v));
        check_range(cc::make_range_ref<int>(v));
    }
    {
        cc::array<int> v = {1, 2, 3};
        check_range(v);
        check_range(cc::make_range_ref(v));
        check_range(cc::make_range_ref<int>(v));
    }
    {
        check_range({1, 2, 3});
        check_range(cc::make_range_ref({1, 2, 3}));
        check_range(cc::make_range_ref<int>({1, 2, 3}));
    }
}

TEST("cc::range_ref conversion")
{
    auto check_range = [](cc::range_ref<cc::string_view> r, cc::string_view result) {
        cc::string s;
        r.for_each([&](cc::string_view sv) {
            if (!s.empty())
                s += ' ';
            s += sv;
        });
        CHECK(s == result);
    };

    {
        cc::vector<cc::string> words = {"brave", "new", "world"};
        check_range(words, "brave new world");
        check_range(cc::make_range_ref(words), "brave new world");
        check_range(cc::make_range_ref<cc::string_view>(words), "brave new world");
    }
    {
        cc::array<cc::string_view> words = {"brave", "new", "world"};
        check_range(words, "brave new world");
        check_range(cc::make_range_ref(words), "brave new world");
        check_range(cc::make_range_ref<cc::string_view>(words), "brave new world");
    }
    {
        char const* words[] = {"brave", "new", "world"};
        check_range(words, "brave new world");
        check_range(cc::make_range_ref(words), "brave new world");
        check_range(cc::make_range_ref<cc::string_view>(words), "brave new world");
    }
    {
        check_range({"brave", "new", "world"}, "brave new world");
        check_range(cc::make_range_ref({"brave", "new", "world"}), "brave new world");
        check_range(cc::make_range_ref<cc::string_view>({"brave", "new", "world"}), "brave new world");
    }
}

TEST("cc::range_ref deref conversion")
{
    int a = 7;
    int b = 3;
    cc::vector<int*> vals;
    vals.push_back(&a);
    vals.push_back(&b);
    vals.push_back(&a);

    auto check_range = [](cc::range_ref<int> vals, int result) {
        auto sum = 0;
        vals.for_each([&](int v) { sum += v; });
        CHECK(sum == result);
    };

    check_range(vals, 17);
    check_range({1, 2, 3, 4, 5}, 15);
    check_range({&a, &b, &a, &a}, 24);
}

namespace
{
struct foo
{
};
struct bar
{
};

int count_objs(cc::range_ref<foo const&> range)
{
    auto cnt = 0;
    range.for_each([&](foo const&) { ++cnt; });
    return cnt;
}
int count_objs(cc::range_ref<bar const&> range = {})
{
    auto cnt = 0;
    range.for_each([&](bar const&) { ++cnt; });
    return cnt + 10;
}
}

TEST("cc::range_ref overloads")
{
    cc::vector<foo> foos;
    cc::vector<bar> bars;

    foos.emplace_back();
    foos.emplace_back();

    bars.emplace_back();
    bars.emplace_back();
    bars.emplace_back();

    cc::vector<cc::unique_ptr<foo>> pfoos;
    cc::vector<cc::unique_ptr<bar>> pbars;

    pfoos.push_back(cc::make_unique<foo>());
    pfoos.push_back(cc::make_unique<foo>());
    pfoos.push_back(cc::make_unique<foo>());

    pbars.push_back(cc::make_unique<bar>());
    pbars.push_back(cc::make_unique<bar>());

    CHECK(count_objs() == 10); // calls bar version

    CHECK(count_objs(foos) == 2);
    CHECK(count_objs(bars) == 13);

    CHECK(count_objs(pfoos) == 3);
    CHECK(count_objs(pbars) == 12);

    CHECK(count_objs({foo{}, foo{}}) == 2);
    CHECK(count_objs({bar{}, bar{}}) == 12);

    CHECK(count_objs({&foos[0], &foos[1], &foos[0]}) == 3);
    CHECK(count_objs({&bars[0], &bars[1]}) == 12);

    CHECK(count_objs(cc::range_ref<foo const&>()) == 0);
    CHECK(count_objs(cc::range_ref<bar const&>()) == 10);
}
