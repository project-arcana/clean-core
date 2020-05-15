#include "format.hh"

#include <clean-core/char_predicates.hh>

void cc::detail::vformat_to(cc::string_stream& ss, cc::string_view fmt_str, cc::span<arg_info> args)
{
    ss.reserve(fmt_str.size());

    // index of the next argument
    // Cannot be used after a named or numbered arg was encountered: -1 == invalid
    int arg_id = 0;

    // each iteration handle one argument (if there are any)
    auto char_iter = fmt_str.begin();
    while (char_iter != fmt_str.end())
    {
        auto segment_start = char_iter;
        // find next occurence of '{', or end, and all occurrences of "}}"
        while (char_iter != fmt_str.end() && *char_iter != '{')
        {
            if (*char_iter == '}')
            {
                ss << cc::string_view(segment_start, char_iter);
                ++char_iter;
                CC_ASSERT(char_iter != fmt_str.end() && *char_iter == '}' && "Invalid format string: Unmatched }");
                segment_start = char_iter;
            }
            ++char_iter;
        }

        // append current segment
        if (char_iter != segment_start)
            ss << cc::string_view(segment_start, char_iter);

        // nothing to replace
        if (char_iter == fmt_str.end())
            return;

        // char_iter now points to '{'
        ++char_iter;
        CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
        if (*char_iter == '{') // escape second {
            ss << cc::string_view(char_iter, char_iter + 1);
        else if (*char_iter == '}')
        {
            // case {}
            CC_ASSERT(arg_id >= 0 && "Invalid format string: Cannot use {} after named or indexed argument");
            args[arg_id].do_format(ss, args[arg_id].data, {});
            ++arg_id;
        }
        else
        {
            // optionally resolve index / name lookup
            size_t argument_index = -1;
            if (is_digit(*char_iter)) // number
            {
                // todo: are leading zeros valid?
                size_t index = *char_iter - '0';
                ++char_iter;
                CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                while (is_digit(*char_iter))
                {
                    index *= 10;
                    index += *char_iter - '0';
                    ++char_iter;
                    CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                }
                CC_ASSERT(index < args.size() && "Invalid format string: Argument index too large");
                argument_index = index;
                arg_id = -1; // invalidate
            }
            else if (*char_iter == '_' || is_lower(*char_iter) || is_upper(*char_iter)) // named
            {
                auto const name_start = char_iter;
                ++char_iter;
                CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                while (*char_iter == '_' || is_lower(*char_iter) || is_upper(*char_iter) || is_digit(*char_iter))
                {
                    ++char_iter;
                    CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                }
                auto const name = cc::string_view(name_start, char_iter);
                for (auto i = 0; i < args.size(); ++i)
                {
                    if (args[i].name != nullptr && cc::string_view(args[i].name) == name)
                    {
                        argument_index = i;
                        break;
                    }
                }
                arg_id = -1; // invalidate
                CC_ASSERT(argument_index < args.size() && "Invalid format string: Argument name not found");
            }
            CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
            if (*char_iter == '}')
            {
                // we can only reach this if either a named or indexed argument is parsed
                args[argument_index].do_format(ss, args[argument_index].data, {});
            }
            else
            {
                CC_ASSERT(*char_iter == ':' && "Invalid format string: Missing closing }");
                ++char_iter;
                CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                auto const args_start = char_iter;
                while (*char_iter != '}')
                {
                    ++char_iter;
                    CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                }
                // todo: handle arguments that themselves contain args
                args[argument_index].do_format(ss, args[argument_index].data, cc::string_view(args_start, char_iter));
            }
        }
        ++char_iter;
    }
}
