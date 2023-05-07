#pragma once

#include <cstdio>

#include <clean-core/format.hh>

namespace cc
{
/// prints the given formatted string to stdout
template <class Formatter = detail::default_formatter, class... Args>
void print(char const* fmt_str, Args const&... args)
{
    ::fputs(cc::format(fmt_str, args...).c_str(), stdout);
}
void print(char const* s);
void print(cc::string_view s);
void print(cc::string const& s);
template <class T>
void print(T const& v)
{
    cc::print("%s", v);
}
/// prints the given formatted string to stdout and appends a \n
/// NOTE: also flushes the stream
///       if this behavior is not desired, use cc::print with \n as part of the format string
template <class Formatter = detail::default_formatter, class... Args>
void println(char const* fmt_str, Args const&... args)
{
    ::puts(cc::format(fmt_str, args...).c_str());
    ::fflush(stdout);
}
void println(char const* s);
void println(cc::string_view s);
void println(cc::string const& s);
template <class T>
void println(T const& v)
{
    cc::println("%s", v);
}
}
