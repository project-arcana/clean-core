#include <nexus/test.hh>

#include <clean-core/stream_ref.hh>
#include <clean-core/string.hh>
#include <clean-core/string_stream.hh>
#include <clean-core/vector.hh>

TEST("cc::stream_ref<int>")
{
    auto foo = [](cc::stream_ref<int> s) {
        s << 1;
        int v[] = {2, 3};
        s << v;
    };

    // lambda ctor
    {
        cc::vector<int> v;
        foo(cc::stream_ref<int>{[&v](cc::span<int const> ii) {
            for (auto i : ii)
                v.push_back(i);
        }});
        CHECK(v == cc::vector<int>{1, 2, 3});
    }
    {
        cc::vector<int> v;
        foo(cc::stream_ref<int>{[&v](cc::span<int const> ii) {
            for (auto i : ii)
                v.push_back(i);
        }});
        CHECK(v == cc::vector<int>{1, 2, 3});
    }

    // lambda
    {
        cc::vector<int> v;
        foo([&v](cc::span<int const> ii) {
            for (auto i : ii)
                v.push_back(i);
        });
        CHECK(v == cc::vector<int>{1, 2, 3});
    }
    {
        cc::vector<int> v;
        foo([&v](cc::span<int const> ii) {
            for (auto i : ii)
                v.push_back(i);
        });
        CHECK(v == cc::vector<int>{1, 2, 3});
    }

    // make_stream_ref
    {
        cc::vector<int> v;
        foo(cc::make_stream_ref<int>([&v](int i) { v.push_back(i); }));
        CHECK(v == cc::vector<int>{1, 2, 3});
    }
    {
        cc::vector<int> v;
        foo(cc::make_stream_ref<int>([&v](cc::span<int const> ii) {
            for (auto i : ii)
                v.push_back(i);
        }));
        CHECK(v == cc::vector<int>{1, 2, 3});
    }
}

TEST("cc::stream_ref<char>")
{
    auto foo = [](cc::stream_ref<char> s) {
        s << 'a';
        char v[] = {'b', 'c'};
        s << v;
        s << cc::string_view("def"); // NOTE: string literal directly would add a null char!
        s << cc::string_view("gh");
        s << cc::string("ijk");
        cc::string ss = "l";
        s << ss;
        cc::string_view sv = "m";
        s << sv;
    };

    // make_stream_ref
    {
        cc::string s;
        foo(cc::make_stream_ref<char>([&s](char c) { s += c; }));
        CHECK(s == "abcdefghijklm");
    }
    {
        cc::string s;
        foo(cc::make_stream_ref<char>([&s](cc::span<char const> ii) { s += ii; }));
        CHECK(s == "abcdefghijklm");
    }
    {
        cc::string_stream s;
        foo(cc::make_stream_ref<char>(s));
        CHECK(s.to_string() == "abcdefghijklm");
    }
}

TEST("cc::string_stream_ref")
{
    auto foo = [](cc::string_stream_ref s) {
        s << 'a';
        char v[] = {'b', 'c', '\0'}; // NOTE: must be null-terminated!
        s << v;
        s << "def";
        s << cc::string_view("gh");
        s << cc::string("ijk");
        cc::string ss = "l";
        s << ss;
        cc::string_view sv = "m";
        s << sv;
    };

    // make_stream_ref
    {
        cc::string s;
        foo(cc::make_string_stream_ref([&s](char c) { s += c; }));
        CHECK(s == "abcdefghijklm");
    }
    {
        cc::string s;
        foo(cc::make_string_stream_ref([&s](cc::span<char const> ii) { s += ii; }));
        CHECK(s == "abcdefghijklm");
    }
    {
        cc::string_stream s;
        foo(cc::make_string_stream_ref(s));
        CHECK(s.to_string() == "abcdefghijklm");
    }
}

TEST("cc::stream_ref<char> - char array")
{
    auto foo = [](cc::stream_ref<char> s) {
        char v[] = {char(10), char(0), char(17)};
        s << v;
    };

    cc::vector<char> v;
    foo(cc::make_stream_ref<char>([&](char c) { v.push_back(c); }));

    CHECK(v == cc::vector<char>{char(10), char(0), char(17)});
}
