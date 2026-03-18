#include "demangle.hh"

#include <clean-core/temp_cstr.hh>

#if defined(CC_OS_LINUX) || defined(CC_OS_APPLE)
#include <cxxabi.h>
#include <cstdlib>

cc::string cc::demangle(string_view mangled_name)
{
    int status = -4;
    // TODO: use non-allocating version if performance is ever a concern
    auto cname = abi::__cxa_demangle(cc::temp_cstr(mangled_name), nullptr, nullptr, &status);

    if (!cname)
        return mangled_name; // some error occurred

    auto name = cc::string(cname);
    ::free(cname);

    return name;
}

#else
// TODO: implement me
cc::string cc::demangle(string_view mangled_name) { return mangled_name; }
#endif
