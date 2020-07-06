#pragma once

#include <type_traits>

#include <clean-core/function_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string_stream.hh>
#include <clean-core/string_view.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
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

struct default_formatter
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
            s << string_view(to_string(v));
        }
        else if constexpr (detail::has_member_to_string<T>)
        {
            s << string_view(v.to_string());
        }
        else
        {
            static_assert(cc::always_false<T>, "Type requires a to_string() function");
        }
    }
};

struct arg_info
{
    function_ptr<void(stream_ref<char>, void const*, string_view)> do_format;
    void const* data = nullptr;
    string_view name;
};

template <class Formatter = default_formatter, class T>
arg_info make_arg_info(T const& v)
{
    return {[](stream_ref<char> s, void const* data, string_view options) -> void { Formatter::do_format(s, *static_cast<T const*>(data), options); }, &v};
}

template <class Formatter = default_formatter, class T>
arg_info make_arg_info(format_arg<T> const& a)
{
    return {[](stream_ref<char> ss, void const* data, string_view options) -> void { Formatter::do_format(ss, *static_cast<T const*>(data), options); },
            &a.value, a.name};
}

void vformat_to(stream_ref<char> s, string_view fmt_str, span<arg_info const> args);
}

template <class T>
struct format_arg
{
    format_arg(string_view name, T const& v) : name{name}, value{v} {}
    string_view name;
    T const& value;
};

template <class Formatter = detail::default_formatter, class... Args>
void format_to(stream_ref<char> s, string_view fmt_str, Args const&... args)
{
    if constexpr (sizeof...(args) == 0)
    {
        vformat_to(s, fmt_str, {});
    }
    else
    {
        detail::arg_info vargs[] = {detail::make_arg_info(args)...};
        vformat_to(s, fmt_str, vargs);
    }
}

template <class Formatter = detail::default_formatter, class... Args>
string format(char const* fmt_str, Args const&... args)
{
    string_stream ss;
    format_to<Formatter>(make_stream_ref<char>(ss), fmt_str, args...);
    return ss.to_string();
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
