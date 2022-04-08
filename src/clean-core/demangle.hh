#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>

namespace cc
{
cc::string demangle(cc::string_view mangled_name);
}
