#include <nexus/monte_carlo_test.hh>

#include <string>

#include <clean-core/string.hh>

MONTE_CARLO_TEST("cc::string mct")
{
    auto const make_char = [](tg::rng& rng) { return uniform(rng, 'A', 'z'); };

    addOp("gen char", make_char);

    auto const addType = [&](auto obj, bool is_cc) {
        using string_t = decltype(obj);

        addOp("default ctor", [] { return string_t(); });
        addOp("copy ctor", [](string_t const& s) { return string_t(s); });
        addOp("move ctor", [](string_t const& s) { return cc::move(string_t(s)); });
        addOp("copy assignment", [](string_t& a, string_t const& b) { a = b; });
        addOp("move assignment", [](string_t& a, string_t const& b) { a = string_t(b); });

        addOp("randomize", [&](tg::rng& rng, string_t& s) {
            auto cnt = uniform(rng, 0, 30);
            s.resize(cnt);
            for (auto i = 0; i < cnt; ++i)
                s[i] = make_char(rng);
            return s;
        });

        addOp("reserve", [](tg::rng& rng, string_t& s) { s.reserve(uniform(rng, 0, 30)); });
        addOp("resize", [](tg::rng& rng, string_t& s) { s.resize(uniform(rng, 0, 30)); });
        addOp("resize + char", [](tg::rng& rng, string_t& s, char c) { s.resize(uniform(rng, 0, 30), c); });

        addOp("random replace", [&](tg::rng& rng, string_t& s) { random_choice(rng, s) = make_char(rng); }).when([](tg::rng&, string_t const& s) {
            return s.size() > 0;
        });

        addOp("push_back", [](string_t& s, char c) { s.push_back(c); });

        addOp("op[]", [](tg::rng& rng, string_t const& s) { return random_choice(rng, s); }).when([](tg::rng&, string_t const& s) {
            return s.size() > 0;
        });
        addOp("data[]", [](tg::rng& rng, string_t const& s) {
            return s.data()[uniform(rng, 0, int(s.size()) - 1)];
        }).when([](tg::rng&, string_t const& s) { return s.size() > 0; });

        addOp("fill", [](string_t& s, char v) {
            for (auto& c : s)
                c = v;
        });

        addOp("shrink_to_fit", &string_t::shrink_to_fit);
        addOp("clear", &string_t::clear);

        addOp("size", &string_t::size);
        addOp("front", [](string_t const& s) { return s.front(); }).when_not(&string_t::empty);
        addOp("back", [](string_t const& s) { return s.back(); }).when_not(&string_t::empty);

        addOp("+= char", [](string_t& s, char c) { s += c; });
        addOp("+= string", [](string_t& s, string_t const& rhs) { s += rhs; });
        addOp("+= lit", [](string_t& s) { s += "hello"; });

        addOp("s + s", [](string_t const& a, string_t const& b) { return a + b; });
        addOp("s + c", [](string_t const& a, char b) { return a + b; });
        addOp("s + lit", [](string_t const& a) { return a + "test"; });
        addOp("lit + s", [](string_t const& a) { return "test" + a; });

        if (is_cc)
            addInvariant("cap", [](string_t const& s) { REQUIRE(s.capacity() >= 15); });
    };

    addType(std::string(), false);
    addType(cc::string(), true);

    testEquivalence<std::string, cc::string>();
}

TEST("cc::string processing")
{
    cc::string s;

    s = "foo";
    s.pad_end(5, '_');
    CHECK(s == "foo__");

    s = "foo";
    s.pad_start(5, '_');
    CHECK(s == "__foo");

    s = "too long";
    s.pad_start(5);
    s.pad_end(5);
    CHECK(s == "too long");

    auto const replaced = [](cc::string s, cc::string_view old, cc::string_view replacement) {
        auto refs = s.replaced(old, replacement);
        s.replace(old, replacement);
        CHECK(s == refs);
        return s;
    };
    auto const ireplaced = [](cc::string s, size_t pos, size_t count, cc::string_view replacement) {
        auto refs = s.replaced(pos, count, replacement);
        s.replace(pos, count, replacement);
        CHECK(s == refs);
        return s;
    };
    auto const creplaced = [](cc::string s, char old, char replacement) {
        auto refs = s.replaced(old, replacement);
        s.replace(old, replacement);
        CHECK(s == refs);
        return s;
    };

    CHECK(creplaced("hello", 'l', 'x') == "hexxo");
    CHECK(creplaced("hello", 'c', 'x') == "hello");
    CHECK(creplaced("hello", 'h', 'x') == "xello");

    CHECK(replaced("hello", "l", "") == "heo");
    CHECK(replaced("hello", "x", "") == "hello");
    CHECK(replaced("hello", "ello", "ola") == "hola");
    CHECK(replaced("hello", "l", "ll") == "hellllo");
    CHECK(replaced("hello", "l", "r") == "herro");
    CHECK(replaced("hello", "e", "ello") == "hellollo");
    CHECK(replaced("hello", "hello", "bla") == "bla");
    CHECK(replaced("hello", "h", "hh") == "hhello");
    CHECK(replaced("", "h", "hh") == "");

    CHECK(ireplaced("hello", 0, 0, "abc") == "abchello");
    CHECK(ireplaced("hello", 3, 0, "abc") == "helabclo");
    CHECK(ireplaced("hello", 5, 0, "abc") == "helloabc");
    CHECK(ireplaced("hello", 1, 1, "a") == "hallo");
    CHECK(ireplaced("hello", 1, 1, "") == "hllo");
    CHECK(ireplaced("hello", 1, 1, "aaa") == "haaallo");
    CHECK(ireplaced("hello", 2, 2, "r") == "hero");

    s = "hello";
    CHECK(s.removed_prefix(2) == "llo");
    CHECK(s.removed_prefix(5) == "");
    CHECK(s.removed_suffix(2) == "hel");
    CHECK(s.removed_suffix(5) == "");
    CHECK(s.removed_prefix("hel") == "lo");
    CHECK(s.removed_prefix("") == "hello");
    CHECK(s.removed_prefix("hello") == "");
    CHECK(s.removed_suffix("llo") == "he");
    CHECK(s.removed_suffix("") == "hello");
    CHECK(s.removed_suffix("hello") == "");

    s = "  bla   ";
    CHECK(s.trimmed() == "bla");
    CHECK(s.trimmed_start() == "bla   ");
    CHECK(s.trimmed_end() == "  bla");

    s = "--bla---";
    CHECK(s.trimmed('-') == "bla");
    CHECK(s.trimmed_start('-') == "bla---");
    CHECK(s.trimmed_end('-') == "--bla");

    s = "--bla---";
    s.trim_start('-');
    CHECK(s == "bla---");

    s = "--bla---";
    s.trim_end('-');
    CHECK(s == "--bla");

    s = "--bla---";
    s.trim('-');
    CHECK(s == "bla");

    s = "hello";
    s.fill('x');
    CHECK(s == "xxxxx");
    s.fill('a', 2);
    CHECK(s == "aa");
    s.fill('b', 0);
    CHECK(s == "");
    s.fill('-', 3);
    CHECK(s == "---");

    s = "aBcD";
    CHECK(s.to_lower() == "abcd");
    CHECK(s.to_upper() == "ABCD");
    CHECK(s.capitalized() == "Abcd");
    s.capitalize();
    CHECK(s == "Abcd");

    for (auto ts : {"", " ", " s", "s", "s ", "  abc", "   abc  ", "abc  ", " a bc "})
    {
        {
            s = ts;
            auto r = s.trimmed_start();
            s.trim_start();
            CHECK(r == s);
        }
        {
            s = ts;
            auto r = s.trimmed_end();
            s.trim_end();
            CHECK(r == s);
        }
        {
            s = ts;
            auto r = s.trimmed();
            s.trim();
            CHECK(r == s);
        }
    }

    for (auto n = 0; n <= 5; ++n)
    {
        {
            s = "hello";
            auto rs = s.removed_prefix(n);
            s.remove_prefix(n);
            CHECK(s == rs);
        }
        {
            s = "hello";
            auto rs = s.removed_suffix(n);
            s.remove_suffix(n);
            CHECK(s == rs);
        }
        {
            s = "hello";
            CHECK(s.first(n) == cc::string_view(s).first(n));
            CHECK(s.last(n) == cc::string_view(s).last(n));
        }
    }
}

TEST("cc::string / string_view / char const* equality")
{
    cc::string s0 = "test";
    cc::string_view s1 = "test";
    char const s2[] = "test";
    char const* s3 = "test";

    CHECK(s0 == s0);
    CHECK(s0 == s1);
    CHECK(s0 == s2);
    CHECK(s0 == s3);

    CHECK(s1 == s0);
    CHECK(s1 == s1);
    CHECK(s1 == s2);
    CHECK(s1 == s3);

    CHECK(s2 == s0);
    CHECK(s2 == s1);
    CHECK(s2 == s2); // pointer check
    // NOT EQUAL (by design)! CHECK(s2 == s3);

    CHECK(s3 == s0);
    CHECK(s3 == s1);
    // NOT EQUAL (by design)! CHECK(s3 == s2);
    CHECK(s3 == s3); // pointer check
}
