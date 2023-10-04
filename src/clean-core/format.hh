#pragma once

#include <cstddef>
#include <cstdint>

#include <type_traits>

#include <clean-core/function_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string_stream.hh>
#include <clean-core/string_view.hh>
#include <clean-core/to_string.hh>

namespace cc
{
namespace detail
{
struct default_formatter;
struct printf_formatter;
struct pythonic_formatter;
}

/// a general purpose string interpolation function (aka a "string formatting utility")
/// basically clean-core's version of printf and std::fmt
///
/// Usage:
///
///   // pythonic syntax
///   auto s = cc::format("{} -> {}", 17, true);
///
///   // printf-style syntax
///   // %s will always stringify, even if types mismatch
///   auto s = cc::format("%d -> %s", 17, true);
///
///   // different order
///   auto s = cc::format("{1} {0}!", "World", "Hello");
///
///   // format strings
///   auto s = cc::format("{.2f} %6d", 1.234, 1000);
///
template <class Formatter = detail::default_formatter, class... Args>
[[nodiscard]] string format(char const* fmt_str, Args const&... args);

/// same as cc::format but only uses printf-style % syntax (with the generic %s)
template <class... Args>
[[nodiscard]] string formatf(char const* fmt_str, Args const&... args);

/// same as cc::format but only uses pythonic {} syntax
template <class... Args>
[[nodiscard]] string formatp(char const* fmt_str, Args const&... args);

/// version of cc::format that appends the result to a stream or string
/// this is usually a bit more efficient than cc::format
/// with a custom stream_ref, this function does not allocate
template <class Formatter = detail::default_formatter, class... Args>
void format_to(stream_ref<char> s, string_view fmt_str, Args const&... args);
template <class Formatter = detail::default_formatter, class... Args>
void format_to(string& s, string_view fmt_str, Args const&... args);


//
// Implementation
//

template <class T>
struct format_arg;

namespace detail
{
template <class T, class = std::void_t<>>
struct has_to_string_ss_args_t : std::false_type
{
};
template <class T>
struct has_to_string_ss_args_t<T, std::void_t<decltype(to_string(std::declval<stream_ref<char>>(), std::declval<T>(), std::declval<string_view>()))>> : std::true_type
{
};
template <class T>
constexpr bool has_to_string_ss_args = has_to_string_ss_args_t<T>::value;

template <class T, class = std::void_t<>>
struct has_to_string_ss_t : std::false_type
{
};
template <class T>
struct has_to_string_ss_t<T, std::void_t<decltype(to_string(std::declval<stream_ref<char>>(), std::declval<T>()))>> : std::true_type
{
};
template <class T>
constexpr bool has_to_string_ss = has_to_string_ss_t<T>::value;

template <class T, class = std::void_t<>>
struct has_to_string_args_t : std::false_type
{
};
template <class T>
struct has_to_string_args_t<T, std::void_t<decltype(string_view(to_string(std::declval<T>(), std::declval<string_view>())))>> : std::true_type
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
struct has_member_toStdString_t : std::false_type
{
};
template <class T>
struct has_member_toStdString_t<T, std::void_t<decltype(string_view(std::declval<T>().toStdString()))>> : std::true_type
{
};
template <class T>
constexpr bool has_member_toStdString = has_member_toStdString_t<T>::value;

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

struct default_do_format
{
    template <class T>
    static void do_format(stream_ref<char> s, T const& v, string_view fmt_args)
    {
        if constexpr (detail::has_to_string_ss_args<T>)
        {
            to_string(s, v, fmt_args);
        }
        else if constexpr (detail::has_to_string_args<T>)
        {
            if constexpr (detail::has_to_string_ss<T>)
            {
                if (fmt_args.empty())
                    to_string(s, v);
                else
                    s << to_string(v, fmt_args);
            }
            else
            {
                s << string_view(to_string(v, fmt_args));
            }
        }
        else if constexpr (detail::has_to_string_ss<T>)
        {
            to_string(s, v);
        }
        else if constexpr (detail::has_to_string<T>)
        {
            to_string(s, string_view(to_string(v)), fmt_args);
        }
        else if constexpr (detail::has_member_to_string<T>)
        {
            to_string(s, string_view(v.to_string()), fmt_args);
        }
        else if constexpr (detail::has_member_toStdString<T>)
        {
            to_string(s, string_view(v.toStdString()), fmt_args);
        }
        else
        {
            static_assert(cc::always_false<T>, "Type requires a to_string() function");
        }
    }

    using do_format_fptr = function_ptr<void(stream_ref<char>, void const*, string_view)>;

    struct arg_info
    {
        do_format_fptr do_format;
        void const* data = nullptr;
        string_view name;
        bool was_used = false;
    };

    // TODO: maybe pull out the lambda into a template function to reduce symbol size a bit
    template <class T>
    static arg_info make_arg_info(T const& v)
    {
        return {[](stream_ref<char> s, void const* data, string_view options) -> void
                { default_do_format::do_format(s, *static_cast<T const*>(data), options); },
                &v,
                {}};
    }

    template <class T>
    static arg_info make_arg_info(format_arg<T> const& a)
    {
        return {[](stream_ref<char> ss, void const* data, string_view options) -> void
                { default_do_format::do_format(ss, *static_cast<T const*>(data), options); },
                &a.value, a.name};
    }
};

struct default_formatter : default_do_format
{
    static void vformat_to(stream_ref<char> s, string_view fmt_str, span<arg_info> args);
};
struct printf_formatter : default_do_format
{
    static void vformat_to(stream_ref<char> s, string_view fmt_str, span<arg_info> args);
};
struct pythonic_formatter : default_do_format
{
    static void vformat_to(stream_ref<char> s, string_view fmt_str, span<arg_info> args);
};
}

template <class T>
struct format_arg
{
    format_arg(string_view name, T const& v) : name{name}, value{v} {}
    string_view name;
    T const& value;
};

template <class Formatter, class... Args>
void format_to(stream_ref<char> s, string_view fmt_str, Args const&... args)
{
    if constexpr (sizeof...(args) == 0)
    {
        Formatter::vformat_to(s, fmt_str, {});
    }
    else
    {
        typename Formatter::arg_info vargs[] = {Formatter::make_arg_info(args)...};
        Formatter::vformat_to(s, fmt_str, vargs);
    }
}

template <class Formatter, class... Args>
void format_to(string& s, string_view fmt_str, Args const&... args)
{
    cc::format_to([&s](span<char const> ss) { s += cc::string_view(ss.data(), ss.size()); }, fmt_str, args...);
}

template <class Formatter, class... Args>
string format(char const* fmt_str, Args const&... args)
{
    string_stream ss;
    format_to<Formatter>(make_stream_ref<char>(ss), fmt_str, args...);
    return ss.to_string();
}

template <class... Args>
string formatf(char const* fmt_str, Args const&... args)
{
    return cc::format<detail::printf_formatter>(fmt_str, args...);
}

template <class... Args>
string formatp(char const* fmt_str, Args const&... args)
{
    return cc::format<detail::pythonic_formatter>(fmt_str, args...);
}

namespace format_literals
{
namespace detail
{
struct arg_capture
{
    string_view name;
    template <class T>
    cc::format_arg<T> operator=(T const& rhs)
    {
        return cc::format_arg(name, rhs);
    }
};
}
inline detail::arg_capture operator"" _a(const char* name, std::size_t size) { return {{name, size}}; }
}
}
