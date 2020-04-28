#include "format.hh"

void cc::vformat_to(string_stream& ss, string_view fmt_str, span<arg_info> args)
{
    auto const is_digit = [](unsigned char c) -> bool { return '0' <= c && c <= '9'; };
    auto const is_letter = [](unsigned char c) -> bool { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); };

    // todo: benchmark if useful
    ss.reserve(fmt_str.size());

    // if not explicityly given, take the next argument.
    // Cannot be used after a named or numbered arg was encountered
    int arg_id = 0;

    // each iteration handle one argument (if there are any)
    auto char_iter = fmt_str.begin();
    while (char_iter != fmt_str.end())
    {
        auto segment_start = char_iter;
        // find next occurence of '{', or end, and all occurrences of }}
        while (*char_iter != '{' && char_iter != fmt_str.end())
        {
            if (*char_iter == '}')
            {
                ss << string_view(segment_start, char_iter);
                ++char_iter;
                CC_ASSERT(*char_iter == '}' && char_iter != fmt_str.end() && "Invalid format string: Unmatched }");
                segment_start = char_iter;
            }
            ++char_iter;
        }

        // append current segment
        if (char_iter != segment_start)
            ss << string_view(segment_start, char_iter);

        // nothing to replace
        if (char_iter == fmt_str.end())
            return;

        // else it now points to {
        ++char_iter;
        CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
        if (*char_iter == '{') // escape
            ss << string_view(char_iter, char_iter + 1);
        else if (*char_iter == '}')
        {
            // case {}
            CC_ASSERT(arg_id >= 0 && "Cannot use {} after named or indexed argument");
            args[arg_id].do_format(ss, args[arg_id].data, {});
            ++arg_id;
        }
        else
        {
            // either number or named argument or parse args
            size_t argument_index = -1;
            if (is_digit(*char_iter)) // number
            {
                // todo: are leading zeros valid?
                size_t index = *char_iter - '0';
                ++char_iter;
                while (is_digit(*char_iter))
                {
                    index *= 10;
                    index += *char_iter - '0';
                    ++char_iter;
                }
                CC_ASSERT(index < args.size() && "Invalid format string: index too large");
                argument_index = index;
                arg_id = -1; // invalidate
            }
            else if (*char_iter == '_' || is_letter(*char_iter)) // named
            {
                auto const name_start = char_iter;
                ++char_iter;
                while (*char_iter == '_' || is_letter(*char_iter) || is_digit(*char_iter))
                {
                    ++char_iter;
                    CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: index too large");
                }
                auto const name = string_view(name_start, char_iter);
                for (auto i = 0; i < args.size(); ++i)
                {
                    if (args[i].name != nullptr && string_view(args[i].name) == name)
                    {
                        argument_index = i;
                        break;
                    }
                }
                arg_id = -1; // invalidate
                CC_ASSERT(argument_index < args.size() && "Invalid format string: argument name not found");
            }

            if (*char_iter == '}')
            {
                // parse arg
                args[argument_index].do_format(ss, args[argument_index].data, {});
            }
            else
            {
                CC_ASSERT(*char_iter == ':' && "Invalid format string: Missing closing }");
                ++char_iter;
                auto args_start = char_iter;
                while (*char_iter != '}' && char_iter != fmt_str.end())
                    ++char_iter;
                CC_ASSERT(char_iter != fmt_str.end() && "Invalid format string: Missing closing }");
                // todo: this is double parsing!
                // todo: handle arguments that themselves contain args!
                args[argument_index].do_format(ss, args[argument_index].data, string_view(args_start, char_iter));
            }
        }
        ++char_iter;
    }
}
