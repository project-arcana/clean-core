#include "print.hh"

#include <clean-core/temp_cstr.hh>

void cc::print(string_view s) { ::fputs(cc::temp_cstr(s), stdout); }
void cc::print(char const* s) { ::fputs(s, stdout); }
void cc::print(cc::string const& s) { ::fputs(s.c_str(), stdout); }

void cc::println(string_view s)
{
    ::puts(cc::temp_cstr(s));
    ::fflush(stdout);
}
void cc::println(char const* s)
{
    ::puts(s);
    ::fflush(stdout);
}
void cc::println(cc::string const& s)
{
    ::puts(s.c_str());
    ::fflush(stdout);
}
