#include <nexus/test.hh>

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

static_assert(cc::is_contiguous_range<cc::string_view, char const>);
static_assert(cc::is_contiguous_range<cc::string_view, void const>);

TEST("cc::string_view")
{
    cc::string_view s;
    CHECK(s.empty());

    s = "hello";
    CHECK(s[0] == 'h');
    CHECK(s.size() == 5);
    CHECK(s[4] == 'o');
    CHECK(s == "hello");
    CHECK(s.starts_with(""));
    CHECK(s.starts_with("hel"));
    CHECK(s.starts_with("hello"));
    CHECK(!s.starts_with("hellos"));
    CHECK(!s.starts_with("hels"));
    CHECK(s.ends_with(""));
    CHECK(s.ends_with("lo"));
    CHECK(s.ends_with("hello"));
    CHECK(!s.ends_with("hell"));
    CHECK(!s.ends_with("yello"));
    CHECK(!s.ends_with("hhello"));

    s = "  bla   ";
    CHECK(s.trim() == "bla");
    CHECK(s.trim_start() == "bla   ");
    CHECK(s.trim_end() == "  bla");

    s = "--bla---";
    CHECK(s.trim('-') == "bla");
    CHECK(s.trim_start('-') == "bla---");
    CHECK(s.trim_end('-') == "--bla");

    s = "hello";
    CHECK(s.remove_prefix(2) == "llo");
    CHECK(s.remove_prefix(5) == "");
    CHECK(s.remove_suffix(2) == "hel");
    CHECK(s.remove_suffix(5) == "");
    CHECK(s.remove_prefix("hel") == "lo");
    CHECK(s.remove_prefix("") == "hello");
    CHECK(s.remove_prefix("hello") == "");
    CHECK(s.remove_suffix("llo") == "he");
    CHECK(s.remove_suffix("") == "hello");
    CHECK(s.remove_suffix("hello") == "");
    CHECK(s.first(2) == "he");
    CHECK(s.first(5) == "hello");
    CHECK(s.first(0) == "");
    CHECK(s.last(2) == "lo");
    CHECK(s.last(5) == "hello");
    CHECK(s.last(0) == "");

    CHECK(s.contains("hell"));
    CHECK(s.contains("o"));
    CHECK(s.contains("lo"));
    CHECK(s.contains("ll"));
    CHECK(s.contains("h"));
    CHECK(s.contains("he"));
    CHECK(!s.contains("hl"));

    struct foo
    {
        int method(char const* /*c_str*/) { return 0; }
        int method(cc::string_view /*str*/) { return 1; }
    };

    CHECK(foo{}.method("cstr") == 0);
    char const* cstr_lval = "bla";
    CHECK(foo{}.method(cstr_lval) == 0);
    CHECK(foo{}.method(s) == 1);
}

TEST("cc::string_view split")
{
    auto const split_s = [](cc::string_view s, std::initializer_list<cc::string> result, cc::split_options opts = cc::split_options::keep_empty) {
        CHECK(cc::vector<cc::string>(s.split(' ', opts)) == cc::vector<cc::string>(result));
    };
    auto const split = [](cc::string_view s, std::initializer_list<cc::string> result) {
        CHECK(cc::vector<cc::string>(s.split()) == cc::vector<cc::string>(result));
    };

    split_s("", {});
    split_s(" ", {"", ""});
    split_s("abc", {"abc"});
    split_s("hello world", {"hello", "world"});
    split_s(" hello world", {"", "hello", "world"});
    split_s("hello world ", {"hello", "world", ""});
    split_s(" hello world  ", {"", "hello", "world", "", ""});
    split_s("   a  b c", {"", "", "", "a", "", "b", "c"});

    split_s("", {}, cc::split_options::skip_empty);
    split_s(" ", {}, cc::split_options::skip_empty);
    split_s("abc", {"abc"}, cc::split_options::skip_empty);
    split_s("hello world", {"hello", "world"}, cc::split_options::skip_empty);
    split_s(" hello world", {"hello", "world"}, cc::split_options::skip_empty);
    split_s("hello world ", {"hello", "world"}, cc::split_options::skip_empty);
    split_s(" hello world  ", {"hello", "world"}, cc::split_options::skip_empty);
    split_s("   a  b c", {"a", "b", "c"}, cc::split_options::skip_empty);

    split("", {});
    split(" ", {});
    split(" a", {"a"});
    split("ab c ", {"ab", "c"});

    CHECK(cc::vector<cc::string>(cc::string_view("barxolite").split([](char c) { return c == 'a' || c == 'x'; })) == cc::vector<cc::string>{"b", "r", "olite"});
}

TEST("cc::string_view span interop")
{
    cc::string_view sv = "hello";
    cc::span<char const> ss;
    ss = sv;
    CHECK(cc::string(ss) == "hello");

    ss = cc::string_view("world");
    CHECK(cc::string(ss) == "world");

    auto sss0 = cc::span(sv);
    CHECK(sss0.size() == 5);

    auto sss1 = cc::span<char const>(sv);
    CHECK(sss1.size() == 5);
}

TEST("cc::string_view from array")
{
    char v[] = {'a', 'b', '\0'};
    auto sv = cc::string_view(v);
    auto ss = cc::span(v);
    auto ssv = cc::string_view(ss);
    auto svs = cc::span(sv);

    CHECK(sv.size() == 2);
    CHECK(ss.size() == 3);
    CHECK(ssv.size() == 3);
    CHECK(svs.size() == 2);
}
