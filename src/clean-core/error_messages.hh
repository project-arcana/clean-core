#pragma once

#include <initializer_list>

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>

//
// this header contains helper for constructing pleasing error messages
//

namespace cc
{

struct substr_error
{
    cc::string_view target; // can be zero-sized to indicate the space between two chars
    cc::string_view message;

    substr_error() = default;
    substr_error(cc::string_view target, cc::string_view message) : target(target), message(message) {}
    substr_error(char const* target, cc::string_view message) : target(target, size_t(0)), message(message) {}
};

/// creates an ASCII-art error message for 'str'
/// does NOT have a trailing \n
cc::string make_error_message_for_substrings(cc::string_view str, std::initializer_list<substr_error> errors, cc::string_view message = "");

}
