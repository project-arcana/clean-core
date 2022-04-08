#include <nexus/test.hh>

#include <cstdint>

#include <clean-core/array.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/range.hh>

static_assert(std::is_trivially_copyable_v<cc::span<int>>);

TEST("cc::span")
{
    cc::vector<int> v = {1, 2, 3};

    auto s = cc::span(v);
    CHECK(s.size() == 3);
    CHECK(s[0] == 1);
    CHECK(s[2] == 3);

    s = s.subspan(1, 2);
    CHECK(s.size() == 2);
    CHECK(s[0] == 2);
    CHECK(s[1] == 3);

    int va[] = {3, 2, 5, 6};
    s = cc::span(va);
    CHECK(s.size() == 4);
    CHECK(s[0] == 3);
    CHECK(s[3] == 6);
    s[1] += 2;
    CHECK(va[1] == 4);

    int x = 8;
    s = cc::span(x);
    CHECK(s.size() == 1);
    CHECK(s[0] == 8);
    x = 9;
    CHECK(s[0] == 9);

    s = {v.data(), v.size()};
    CHECK(s.size() == 3);
    CHECK(s[0] == 1);
    CHECK(s[2] == 3);

    s = {v.begin(), v.end()};
    CHECK(s.size() == 3);
    CHECK(s[0] == 1);
    CHECK(s[2] == 3);

    s = s.subspan(2);
    CHECK(s.size() == 1);
    CHECK(s[0] == 3);

    s = s.subspan(1);
    CHECK(s.size() == 0);
    CHECK(s.empty());

    s = v;
    CHECK(s.size() == 3);
    CHECK(cr::range(s) == cc::vector{1, 2, 3});

    s = s.first(2);
    CHECK(cr::range(s) == cc::vector{1, 2});

    s = cc::span(v).last(2);
    CHECK(cr::range(s) == cc::vector{2, 3});

    auto b = cc::span(v).as_bytes();
    CHECK(b.size() == 3 * sizeof(int));

    auto wb = cc::span(v).as_writable_bytes();
    CHECK(wb.size() == 3 * sizeof(int));
    wb[3] = std::byte(8);
    CHECK(v[0] == 1 + (8 << 24));

    static_assert(std::is_trivially_copyable_v<decltype(s)>, "span not triv. copyable");
    static_assert(std::is_trivially_copyable_v<cc::span<void*>>, "span not triv. copyable");
    static_assert(std::is_trivially_move_constructible_v<cc::span<void*>>, "span not triv. movable");
    static_assert(std::is_trivially_move_assignable_v<cc::span<void*>>, "span not triv. movable");
}

TEST("cc::span copy")
{
    cc::vector<int> input = {1, 2, 3};
    auto s = cc::span(input);

    cc::array<int, 3> output;
    s.copy_to(output);

    CHECK(output[0] == 1);
    CHECK(output[1] == 2);
    CHECK(output[2] == 3);
}

namespace
{
template <class A, class B>
void check_copy_to(A a, B b)
{
    for (auto& v : b)
        v = -1;

    a.copy_to(b);

    for (auto i = 0u; i < a.size(); ++i)
        CHECK(a[i] == b[i]);
}
template <class A, class B>
void check_copy_from(A a, B b)
{
    for (auto& v : a)
        v = -1;

    a.copy_from(b);

    for (auto i = 0u; i < a.size(); ++i)
        CHECK(a[i] == b[i]);
}
}

TEST("cc::span copy variants")
{
    int va[] = {1, 2, 3};
    int vb[] = {1, 2, 3};
    float vc[] = {1, 2, 3};

    check_copy_to<cc::span<int>, cc::span<int>>(va, vb);
    check_copy_to<cc::span<int const>, cc::span<int>>(va, vb);
    check_copy_from<cc::span<int>, cc::span<int>>(va, vb);
    check_copy_from<cc::span<int>, cc::span<int const>>(va, vb);

    check_copy_to<cc::span<int>, cc::span<float>>(va, vc);
    check_copy_to<cc::span<int const>, cc::span<float>>(va, vc);
    check_copy_from<cc::span<int>, cc::span<float>>(va, vc);
    check_copy_from<cc::span<int>, cc::span<float const>>(va, vc);
}

TEST("byte_span")
{
    {
        uint32_t x = 0x12345678;
        auto s = cc::as_byte_span(x);
        CHECK(s.size() == 4);
        CHECK(s[0] == std::byte(0x78));
        CHECK(s[1] == std::byte(0x56));
        CHECK(s[2] == std::byte(0x34));
        CHECK(s[3] == std::byte(0x12));
        CHECK(cc::from_byte_span<uint32_t>(s) == 0x12345678);
        CHECK(cc::from_byte_span<int32_t>(s) == 0x12345678);
    }
    {
        char c = 'A';
        auto s = cc::as_byte_span(c);
        CHECK(s.size() == 1);
        CHECK(s[0] == std::byte('A'));
        CHECK(cc::from_byte_span<char>(s) == 'A');
    }
    {
        char c[] = {'A', 'B', 'C'};
        auto s = cc::as_byte_span(c);
        CHECK(s.size() == 3);
        CHECK(s[0] == std::byte('A'));
        CHECK(s[1] == std::byte('B'));
        CHECK(s[2] == std::byte('C'));
        s[1] = std::byte('d');
        CHECK(c[1] == 'd');
    }
    {
        cc::vector<uint32_t> v = {1, 2, 3, 4, 5, 6};
        auto s = cc::as_byte_span(v);
        CHECK(s.size() == 4 * 6);
        CHECK(s[0] == std::byte(1));
        CHECK(s[1] == std::byte(0));
        CHECK(s[4] == std::byte(2));
        CHECK(cc::from_byte_span<uint32_t>(s.subspan(8, 4)) == 3);
    }
    {
        cc::string_view sv = "hello";
        auto s = cc::as_byte_span(sv);
        CHECK(s.size() == 5);
        CHECK(s.back() == std::byte('o'));
    }
    {
        char const sa[] = "hello";
        auto s = cc::as_byte_span(sa);
        CHECK(s.size() == 6);
        CHECK(s.back() == std::byte('\0'));
    }
    {
        struct
        {
            char a = 'A';
            char b = 'B';
            short s = 1;
        } f;
        auto s = cc::as_byte_span(f);
        CHECK(s.size() == 4);
        CHECK(s[0] == std::byte('A'));
        CHECK(s[1] == std::byte('B'));
        CHECK(s[2] == std::byte(1));
        CHECK(s[3] == std::byte(0));
    }
}

TEST("cc::span deductions")
{
    {
        cc::vector<int> v;
        auto s = cc::span(v);
        static_assert(std::is_same_v<decltype(s.front()), int&>);
    }
    {
        cc::vector<int> const v;
        auto s = cc::span(v);
        static_assert(std::is_same_v<decltype(s.front()), int const&>);
    }
    {
        cc::array<std::byte> v;
        auto s = cc::span(v);
        static_assert(std::is_same_v<decltype(s.front()), std::byte&>);
    }
    {
        auto s = cc::span(cc::vector<int>{});
        static_assert(std::is_same_v<decltype(s.front()), int&>);
    }
    {
        cc::string_view sv;
        auto s = cc::span(sv);
        static_assert(std::is_same_v<decltype(s.front()), char const&>);
    }
    {
        auto s = cc::span(cc::string_view{});
        static_assert(std::is_same_v<decltype(s.front()), char const&>);
    }

    // this test is used for static asserts
    CHECK(true);
}
