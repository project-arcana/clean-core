#include "print.hh"

#include <clean-core/temp_cstr.hh>

void cc::print(string_view s) { ::fputs(cc::temp_cstr(s), stdout); }

void cc::println(string_view s)
{
    ::puts(cc::temp_cstr(s));
    ::fflush(stdout);
}
