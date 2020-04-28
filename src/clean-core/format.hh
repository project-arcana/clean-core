#pragma once

#include <string> // temporary
#include <type_traits>

#include <clean-core/function_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/string.hh>
#include <clean-core/string_stream.hh>
#include <clean-core/string_view.hh>

namespace cc
{
namespace detail
{
template <class T, class = std::void_t<>>
struct has_to_string_ss_args_t : std::false_type
{
};
template <class T>
struct has_to_string_ss_args_t<T, std::void_t<decltype(to_string(std::declval<string_stream>(), std::declval<T>(), std::declval<string_view>()))>> : std::true_type
{
};
template <class T>
constexpr bool has_to_string_ss_args = has_to_string_ss_args_t<T>::value;

template <class T, class = std::void_t<>>
struct has_to_string_ss_t : std::false_type
{
};
template <class T>
struct has_to_string_ss_t<T, std::void_t<decltype(to_string(std::declval<string_stream>(), std::declval<T>()))>> : std::true_type
{
};
template <class T>
constexpr bool has_to_string_ss = has_to_string_ss_t<T>::value;

template <class T, class = std::void_t<>>
struct has_to_string_args_t : std::false_type
{
};
template <class T>
struct has_to_string_args_t<T, std::void_t<decltype(string_view(o_string(std::declval<T>(), std::declval<string_view>())))>> : std::true_type
{
};
template <class T>
constexpr bool has_to_string_args = has_to_string_ss_t<T>::value;

template <class T, class = std::void_t<>>
struct has_to_string_t : std::false_type
{
};
template <class T>
struct has_to_string_t<T, std::void_t<decltype(string_view(to_string(std::declval<T>())))>> : std::true_type
{
};
template <class T>
constexpr bool has_to_string = has_to_string_t<T>::value;

template <class T, class = std::void_t<>>
struct has_member_to_string_t : std::false_type
{
};
template <class T>
struct has_member_to_string_t<T, std::void_t<decltype(string_view(std::declval<T>().to_string()))>> : std::true_type
{
};
template <class T>
constexpr bool has_member_to_string = has_member_to_string_t<T>::value;
}

// to_string implementations for builtin types
// format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]
// fill        ::=  <a character other than '{' or '}'>
// align       ::=  "<" | ">" | "^"
// sign        ::=  "+" | "-" | " "
// width       ::=  integer | "{" arg_id "}"
// precision   ::=  integer | "{" arg_id "}"
// type        ::=  int_type | "a" | "A" | "c" | "e" | "E" | "f" | "F" | "g" | "G" | "L" | "p" | "s"
// int_type    ::=  "b" | "B" | "d" | "o" | "x" | "X"

template <class T>
void to_string(string_stream& ss, T const* b, string_view fmt_args)
{
    // todo parse
}

void to_string(string_stream& ss, bool b, string_view fmt_args)
{
    // todo parse
}

void to_string(string_stream& ss, string const& s, string_view fmt_args)
{
    // todo parse
}

void to_string(string_stream& ss, char c, string_view fmt_args)
{
    // todo parse
}

void to_string(string_stream& ss, int v, string_view fmt_args)
{
    // todo parse
}

void to_string(string_stream& ss, float v, string_view fmt_args)
{
    // todo parse
}

void to_string(string_stream& ss, double v, string_view fmt_args)
{
    // todo parse
}

struct default_formatter
{
    template <class T>
    static void do_format(string_stream& ss, T const& v, string_view fmt_args)
    {
        if constexpr (detail::has_to_string_ss_args<T>)
        {
            to_string(ss, v, fmt_args);
        }
        else if constexpr (detail::has_to_string_args<T>)
        {
            if constexpr (detail::has_to_string_ss<T>)
            {
                if (fmt_args.empty())
                    to_string(ss, v);
                else
                    ss << to_string(v, fmt_args);
            }
            else
            {
                ss << string_view(to_string(v, fmt_args));
            }
        }
        else if constexpr (detail::has_to_string_ss<T>)
        {
            to_string(ss, v);
        }
        else if constexpr (detail::has_to_string<T>)
        {
            ss << string_view(to_string(v));
        }
        else if constexpr (detail::has_member_to_string<T>)
        {
            ss << string_view(v.to_string());
        }
        else
        {
            static_assert(false, "Type requires a to_string() method");
        }
    }
};

struct arg_info
{
    function_ptr<void(string_stream&, void const*, string_view)> do_format;
    void const* data = nullptr;
    char const* name = nullptr;
};

template <class T>
struct arg
{
    arg(char const* name, T const& v) : name{name}, value{v} {}
    char const* name = nullptr;
    T const& value = {};
};

template <class Formatter = default_formatter, class T>
arg_info make_arg_info(T const& v)
{
    return {[](string_stream& ss, void const* data, string_view options) -> void { Formatter::do_format(ss, *static_cast<T const*>(data), options); }, &v};
}

template <class Formatter = default_formatter, class T>
arg_info make_arg_info(arg<T> const& a)
{
    return {[](string_stream& ss, void const* data, string_view options) -> void { Formatter::do_format(ss, *static_cast<T const*>(data), options); },
            &a.value, a.name};
}

void vformat_to(string_stream& ss, string_view fmt_str, span<arg_info> args);

inline string vformat(string_view fmt_str, span<arg_info> args)
{
    string_stream ss;
    vformat_to(ss, fmt_str, args);
    return ss.to_string();
}

template <class Formatter = default_formatter, class... Args>
void format_to(string_stream& ss, string_view fmt_str, Args const&... args)
{
    if constexpr (sizeof...(args) == 0)
    {
        vformat_to(ss, fmt_str, {});
    }
    else
    {
        arg_info vargs[] = {make_arg_info(args)...};
        vformat_to(ss, fmt_str, vargs);
    }
}

template <class Formatter = default_formatter, class... Args>
string format(char const* fmt_str, Args const&... args)
{
    if constexpr (sizeof...(args) == 0)
    {
        return vformat(fmt_str, {});
    }
    else
    {
        arg_info vargs[] = {make_arg_info(args)...};
        return vformat(fmt_str, vargs);
    }
}

/* Do we want this?
template <class Formatter = default_formatter, class... Args>
void print(FILE* out, char const* fmt_str, Args const&... args)
{
    // todo
}
*/
}
