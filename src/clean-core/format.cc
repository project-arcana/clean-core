#include "format.hh"

#include <clean-core/assertf.hh>
#include <clean-core/char_predicates.hh>
#include <clean-core/error_messages.hh>

#define CC_ASSERT_ERRMSG(cond, ...) CC_ASSERTF(cond, "{}", cc::make_error_message_for_substrings(__VA_ARGS__))

namespace cc::detail
{
template <bool support_printf, bool support_pythonic>
static void impl_vformat_to(cc::stream_ref<char> ss, cc::string_view fmt_str, cc::span<default_do_format::arg_info> args)
{
    // index of the next argument
    // cannot be used after a named or numbered arg was encountered: -1 == invalid
    int curr_arg_id = 0;

    auto curr_c = fmt_str.begin();
    auto segment_start = curr_c; // non-formatted part
    auto const end_c = fmt_str.end();

    // each iteration handles one argument (if there are any)
    while (true)
    {
        if (curr_c == end_c)
            break; // done

        // } must belong to }} here
        if (support_pythonic && *curr_c == '}')
        {
            ++curr_c;
            CC_ASSERT_ERRMSG(curr_c != end_c && *curr_c == '}', fmt_str, {{curr_c, "expected '}'"}});

            ss << cc::string_view(segment_start, curr_c);
            ++curr_c;
            segment_start = curr_c;
            continue;
        }

        // handle {} args
        if (support_pythonic && *curr_c == '{')
        {
            auto const arg_start_c = curr_c;

            // append segment in any case
            if (segment_start != curr_c)
                ss << cc::string_view(segment_start, curr_c);

            ++curr_c;
            CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{curr_c, "expected argument or '{'"}}));

            if (*curr_c == '{') // escaped {
            {
                segment_start = curr_c; // next seg starts with '{'
                ++curr_c;
                continue;
            }

            if (*curr_c == '}') // plain {}
            {
                CC_ASSERTF(curr_arg_id >= 0, "{}",
                           cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c + 1}, "cannot use {} after named or indexed argument"}}));
                CC_ASSERTF(curr_arg_id < int(args.size()), "{}",
                           cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c + 1}, "not enough arguments passed to cc::format"}}));
                args[curr_arg_id].do_format(ss, args[curr_arg_id].data, {});
                args[curr_arg_id].was_used = true;
                ++curr_arg_id;
            }
            else // complex args
            {
                // optionally resolve index / name lookup
                size_t argument_index = -1;
                if (is_digit(*curr_c)) // number
                {
                    // TODO: are leading zeros valid?
                    size_t index = *curr_c - '0';
                    ++curr_c;
                    CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));
                    while (is_digit(*curr_c))
                    {
                        index *= 10;
                        index += *curr_c - '0';
                        ++curr_c;
                        CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));
                    }
                    CC_ASSERTF(index < args.size(), "{}",
                               cc::make_error_message_for_substrings(
                                   fmt_str, {{{arg_start_c, curr_c}, "argument index too large or not enough arguments passed to cc::format"}}));
                    argument_index = index;
                    curr_arg_id = -1; // invalidate
                }
                else if (*curr_c == '_' || is_lower(*curr_c) || is_upper(*curr_c)) // named
                {
                    auto const name_start = curr_c;
                    ++curr_c;
                    CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));
                    while (*curr_c == '_' || is_lower(*curr_c) || is_upper(*curr_c) || is_digit(*curr_c))
                    {
                        ++curr_c;
                        CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));
                    }
                    auto const name = cc::string_view(name_start, curr_c);
                    for (auto i = 0u; i < args.size(); ++i)
                    {
                        if (!args[i].name.empty() && args[i].name == name)
                        {
                            argument_index = i;
                            break;
                        }
                    }
                    curr_arg_id = -1; // invalidate
                    CC_ASSERTF(argument_index < args.size(),
                               "{}", cc::make_error_message_for_substrings(fmt_str, {{name, "named argument not found in arguments passed to cc::format"}}));
                }
                else
                {
                    argument_index = curr_arg_id++;
                }
                CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));

                if (*curr_c == '}') // no format string
                {
                    // we can only reach this if either a named or indexed argument is parsed
                    args[argument_index].do_format(ss, args[argument_index].data, {});
                    args[argument_index].was_used = true;
                }
                else
                {
                    CC_ASSERTF(*curr_c == ':', "{}",
                               cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "format specifier must start with ':'"}}));

                    ++curr_c;
                    CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));

                    auto const args_start = curr_c;
                    while (*curr_c != '}')
                    {
                        ++curr_c;
                        CC_ASSERTF(curr_c != end_c, "{}", cc::make_error_message_for_substrings(fmt_str, {{{arg_start_c, curr_c}, "missing closing '}'"}}));
                    }

                    // TODO: handle arguments that themselves contain args
                    args[argument_index].do_format(ss, args[argument_index].data, cc::string_view(args_start, curr_c));
                    args[argument_index].was_used = true;
                }
            }

            // start next segment
            ++curr_c;
            segment_start = curr_c;
            continue;
        }

        // no arg handling? just advance
        ++curr_c;
    }

    // add final segment
    if (segment_start != end_c)
        ss << cc::string_view(segment_start, end_c);

    for (auto i = 0; i < int(args.size()); ++i)
        CC_ASSERTF(args[i].was_used, "argument nr. {} as not used in format string '{}'", i, fmt_str);
}
}

void cc::detail::default_formatter::vformat_to(cc::stream_ref<char> ss, cc::string_view fmt_str, cc::span<arg_info> args)
{
    detail::impl_vformat_to<true, true>(ss, fmt_str, args);
}

void cc::detail::printf_formatter::vformat_to(cc::stream_ref<char> ss, cc::string_view fmt_str, cc::span<arg_info> args)
{
    detail::impl_vformat_to<true, false>(ss, fmt_str, args);
}

void cc::detail::pythonic_formatter::vformat_to(cc::stream_ref<char> ss, cc::string_view fmt_str, cc::span<arg_info> args)
{
    detail::impl_vformat_to<false, true>(ss, fmt_str, args);
}
