#pragma once

#include <string> // temporary

#include <clean-core/function_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/string.hh>
#include <clean-core/string_stream.hh>
#include <clean-core/string_view.hh>

/*
 * TODO:
 * * No arguments currently not possible
 * * allow formatter customization
 */

namespace cc
{
struct default_formatter
{
    template <class T>
    void to_string(string_stream& ss, T const& v, string_view fmt_args)
    {
        // todo:
        // Select one of
        // cc::string to_string(T const&);
        // cc::string to_string(T const&, cc::string_view fmt_args);
        // void to_string(cc::string_stream&, T const&);
        // void to_string(cc::string_stream&, T const&, cc::string_view fmt_args);
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

template <class T>
arg_info make_arg_info(T const& v)
{
    return {[](string_stream& ss, void const* data, string_view options) -> void {
                // todo take correct version
                ss << to_string(*static_cast<T const*>(data));
            },
            &v};
}

// todo: all builtin types

arg_info make_arg_info(int const& v)
{
    return {[](string_stream& ss, void const* data, string_view options) -> void {
                // todo take correct version
                ss << std::to_string(*static_cast<int const*>(data));
            },
            &v};
}

template <class T>
arg_info make_arg_info(arg<T> const& a)
{
    return {[](string_stream& ss, void const* data, string_view options) -> void {
                // todo take correct version
                ss << to_string(*static_cast<T const*>(data));
            },
            &a.value, a.name};
}


void vformat_to(string_stream& s, string_view fmt_str, span<arg_info> args)
{
    auto const is_digit = [](unsigned char c) -> bool { return '0' <= c && c <= '9'; };
    auto const is_letter = [](unsigned char c) -> bool { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); };

    // todo: benchmark if useful
    s.reserve(fmt_str.size());

    // if not explicityly given, take the next argument
    // todo: what happens when named / numbered args are used before {}
    int arg_id = 0;

    // each iteration handle one argument (if there are any)
    auto it = fmt_str.begin();
    while (it != fmt_str.end())
    {
        auto segment_start = it;
        // find next occurence of '{', or end, and all occurrences of }}
        while (*it != '{' && it != fmt_str.end())
        {
            if (*it == '}')
            {
                s << string_view(segment_start, it);
                ++it;
                CC_ASSERT(*it == '}' && it != fmt_str.end() && "Invalid format string: Unmatched }");
                segment_start = it;
            }
            ++it;
        }

        // append current segment
        if (it != segment_start)
            s << string_view(segment_start, it);

        // nothing to replace
        if (it == fmt_str.end())
            return;

        // else it now points to {
        ++it;
        CC_ASSERT(it != fmt_str.end() && "Invalid format string: Missing closing }");
        if (*it == '{') // escape
            s << string_view(it, it + 1);
        else if (*it == '}')
        {
            // case {}
            args[arg_id].do_format(s, args[arg_id].data, {});
            ++arg_id;
        }
        else
        {
            // either number or named argument or parse args
            size_t argument_index = -1;
            if (is_digit(*it)) // number
            {
                // todo: are leading zeros valid?
                size_t index = *it - '0';
                ++it;
                while (is_digit(*it))
                {
                    index *= 10;
                    index += *it - '0';
                    ++it;
                }
                CC_ASSERT(index < args.size() && "Invalid format string: index too large");
                argument_index = index;
            }
            else if (*it == '_' || is_letter(*it)) // named
            {
                auto const name_start = it;
                ++it;
                while (*it == '_' || is_letter(*it) || is_digit(*it))
                {
                    ++it;
                    CC_ASSERT(it != fmt_str.end() && "Invalid format string: index too large");
                }
                auto const name = string_view(name_start, it);
                for (auto i = 0; i < args.size(); ++i)
                {
                    if (args[i].name != nullptr && string_view(args[i].name) == name)
                    {
                        argument_index = i;
                        break;
                    }
                }
                CC_ASSERT(argument_index < args.size() && "Invalid format string: argument name not found");
            }

            if (*it == '}')
            {
                // parse arg
                args[argument_index].do_format(s, args[argument_index].data, {});
            }
            else
            {
                CC_ASSERT(*it == ':' && "Invalid format string: Missing closing }");
                ++it;
                auto args_start = it;
                while (*it != '}' && it != fmt_str.end())
                    ++it;
                CC_ASSERT(it != fmt_str.end() && "Invalid format string: Missing closing }");
                // todo: this is double parsing!
                args[argument_index].do_format(s, args[argument_index].data, string_view(args_start, it));
            }
        }
        ++it;
    }
}

string vformat(string_view fmt_str, span<arg_info> args)
{
    string_stream ss;
    vformat_to(ss, fmt_str, args);
    return ss.to_string();
}

template <class Formatter = default_formatter, class... Args>
void format_to(string_stream& ss, string_view fmt_str, Args const&... args)
{
    arg_info vargs[] = {make_arg_info(args)...};
    vformat_to(ss, fmt_str, vargs);
}

template <class Formatter = default_formatter, class... Args>
string format(char const* fmt_str, Args const&... args)
{
    arg_info vargs[] = {make_arg_info(args)...};
    return vformat(fmt_str, vargs);
}

/* Do we want this?
template <class Formatter = default_formatter, class... Args>
void print(FILE* out, char const* fmt_str, Args const&... args)
{
    // todo
}
*/
}
