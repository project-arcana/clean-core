#include "error_messages.hh"

#include <clean-core/vector.hh>

cc::string cc::make_error_message_for_substrings(string_view str, std::initializer_list<substr_error> errors, string_view message)
{
    for (auto const& e : errors)
        CC_ASSERT(str.begin() <= e.target.begin() && e.target.end() <= str.end() && "error targets must be substrings of input string");

    cc::string res;

    // TODO: this could be made more efficient
    // TODO: colors
    // TODO: should multiline errors be repeated?
    // TODO: configurable delimiters

    struct err_info
    {
        substr_error err;

        int marker_offset = 0; // how many lines the marked is moved down
        int marker_start = 0;
        int marker_end = 0; // exclusive

        int text_offset = 0; // how many lines the text is moved down
        int text_start = 0;
        int text_end = 0; // exclusive

        bool overlaps_marker(err_info const& r) const
        {
            if (r.marker_end < marker_start - 1)
                return false;
            if (r.marker_start > marker_end)
                return false;
            return true;
        }
        bool overlaps_text(err_info const& r) const
        {
            if (r.text_end < text_start - 1)
                return false;
            if (r.text_start > text_end)
                return false;
            return true;
        }
    };

    // special case for empty string
    if (str.empty())
    {
        res += "<empty string>";
        for (auto const& e : errors)
        {
            res += "\n* ";
            res += e.message;
        }
    }
    else
    {
        cc::vector<err_info> errs;
        cc::vector<cc::string> err_lines;

        // build result line-by-line
        auto first_line = true;
        for (auto line : str.split('\n'))
        {
            // add line to output
            if (first_line)
                first_line = false;
            else
                res += "\n";
            res += "> ";
            res += line;

            // prepare errors
            errs.clear();
            int total_marker_lines = 0;
            int total_text_lines = 0;
            int max_line_size = 0;
            for (auto const& e : errors)
            {
                auto marker_start = e.target.begin() - line.begin();
                auto marker_end = e.target.end() - line.begin(); // exclusive
                if (marker_start == marker_end)
                    marker_end++;

                if (marker_end <= 0)
                    continue; // earlier line

                if (marker_start > int64_t(line.size()))
                    continue; // not this line
                // contains line.size as special case so that \n can be marked

                err_info err;
                err.err = e;
                err.marker_start = marker_start;
                err.marker_end = marker_end;
                err.text_start = (marker_start + marker_end - 1) / 2;
                err.text_end = err.text_start + e.message.size() + 2;

                // allocate marker
                while (true)
                {
                    auto overlaps = false;
                    for (auto const& ee : errs)
                        if (ee.marker_offset == err.marker_offset && ee.overlaps_marker(err))
                        {
                            overlaps = true;
                            break;
                        }

                    if (!overlaps)
                        break;

                    err.marker_offset++;
                }

                // allocate text
                while (true)
                {
                    auto overlaps = false;
                    for (auto const& ee : errs)
                        if (ee.text_offset == err.text_offset && ee.overlaps_text(err))
                        {
                            overlaps = true;
                            break;
                        }

                    if (!overlaps)
                        break;

                    err.text_offset++;
                }

                total_marker_lines = cc::max(total_marker_lines, err.marker_offset + 1);
                total_text_lines = cc::max(total_text_lines, err.text_offset + 1);

                max_line_size = cc::max(max_line_size, err.marker_end);
                max_line_size = cc::max(max_line_size, err.text_end);

                errs.push_back(err);
            }

            // create error text
            if (!errs.empty())
            {
                err_lines.resize(total_marker_lines + total_text_lines);
                for (auto& s : err_lines)
                {
                    s.clear();
                    s.resize(max_line_size, ' ');
                }

                // connectors
                for (auto const& e : errs)
                {
                    auto x = e.text_start;
                    auto y0 = e.marker_offset + 1;
                    auto y1 = total_marker_lines + e.text_offset - 1;
                    for (auto y = y0; y <= y1; ++y)
                        err_lines[y][x] = '|';
                }

                // markers
                for (auto const& e : errs)
                {
                    auto x0 = e.marker_start;
                    auto x1 = e.marker_end;
                    auto y = e.marker_offset;
                    for (auto x = x0; x < x1; ++x)
                        err_lines[y][x] = '^';
                }

                // text
                for (auto const& e : errs)
                {
                    auto x = e.text_start;
                    auto y = total_marker_lines + e.text_offset;
                    err_lines[y][x++] = '*';
                    err_lines[y][x++] = ' ';
                    for (auto c : e.err.message)
                        err_lines[y][x++] = c;
                }

                // add lines to result
                for (auto const& l : err_lines)
                {
                    res += "\n";
                    res += "  "; // for "> "
                    res += l;
                }
            }
        }
    }

    // append optional message
    if (!message.empty())
    {
        res += "\n";
        res += message;
    }

    return res;
}
