#pragma once

#include <clean-core/function_ref.hh>
#include <clean-core/has_operator.hh>
#include <clean-core/span.hh>

// TODO: check how costly this is
#include <clean-core/string_view.hh>

namespace cc
{
/**
 * a type-erased non-owning reference to a stream of T's
 *
 * supports the following operations:
 *
 *   - operator<<(T const&)
 *   - operator<<(span<T const>)
 *
 * NOTES:
 *   - a stream_ref is copyable and cheap but must not outlive the wrapped stream
 *   - make_stream_ref<T>(...) is a helper to create commonly used streams
 *
 * CAUTION:
 *   - stream_ref<char> behaves consistent with stream_ref<T>, which is inconsistent with a string stream
 *     (e.g. s << "string" adds a null char at the end because "string" is char const[7] with a null char)
 *     use cc::string_stream_ref if string-consistent behavior is desired
 */
template <class T>
struct stream_ref
{
    /// empty stream behaves like /dev/null
    stream_ref()
    {
        _append_fun = [](span<T const>) {};
    }

    /// any function taking a span of Ts can be used to create a stream
    template <class F, cc::enable_if<std::is_invocable_r_v<void, F, span<T const>>> = true>
    stream_ref(F&& fun)
    {
        _append_fun = fun;
    }

    // copy
    stream_ref(stream_ref const&) = default;
    stream_ref& operator=(stream_ref const&) = default;

    // stream API
public:
    stream_ref& operator<<(T const& value)
    {
        _append_fun(span<T const>(value));
        return *this;
    }
    stream_ref& operator<<(span<T const> values)
    {
        _append_fun(values);
        return *this;
    }

private:
    cc::function_ref<void(span<T const>)> _append_fun;

    friend struct string_stream_ref;
};

/**
 * a special version of stream_ref<char> that is specialized for (null-terminated) strings
 *
 * char buf[256];
 * snprintf(buf, ...);
 * s << buf; // works, searches first null char in buf
 *
 * char v[] = {'a', 'c'};
 * s << v; // reads out of memory, must add '\0' to v
 *
 * NOTE:
 *   - can still be created via cc::make_stream_ref<char> (or cc::make_string_stream_ref)
 */
struct string_stream_ref : stream_ref<char>
{
    using stream_ref<char>::stream_ref;
    string_stream_ref(stream_ref<char> s) { _append_fun = s._append_fun; }

    string_stream_ref& operator<<(char c)
    {
        stream_ref<char>::operator<<(c);
        return *this;
    }
    string_stream_ref& operator<<(cc::string_view s)
    {
        stream_ref<char>::operator<<(cc::span<char const>(s));
        return *this;
    }
};

/// creates a stream_ref from a given stream
/// CAUTION: stream must outlive the stream_ref!
///
/// The following stream interfaces are supported (tested in order):
///
///   - anything that is invocable<span<T const>>
///   - anything that is invocable<T const&>
///   - anything that supports operator<<(span<T const>)
///   - anything that supports operator<<(T const&)
///
/// NOTE: the result is not technically a stream_ref but should be used where a stream_ref is expected
///       (this is because otherwise lifetime issues with lambdas arise)
template <class T, class Stream>
auto make_stream_ref(Stream&& stream)
{
    if constexpr (std::is_invocable_r_v<void, Stream, span<T const>>)
        return stream_ref<T>(cc::forward<Stream>(stream));

    else if constexpr (std::is_invocable_v<Stream, span<T const>>)
        return [&stream](span<T const> values) { stream(values); };

    else if constexpr (std::is_invocable_v<Stream, T const&>)
        return [&stream](span<T const> values) {
            for (auto const& v : values)
                stream(v);
        };

    else if constexpr (has_operator_left_shift<Stream, span<T const>>)
        return [&stream](span<T const> values) { stream << values; };

    else if constexpr (has_operator_left_shift<Stream, T const&>)
        return [&stream](span<T const> values) {
            for (auto const& v : values)
                stream << v;
        };

    else
        static_assert(cc::always_false<T, Stream>, "no idea how to make stream of given type.");
}

/// shortcut for cc::make_stream_ref<char>
template <class Stream>
auto make_string_stream_ref(Stream&& stream)
{
    return cc::make_stream_ref<char>(cc::forward<Stream>(stream));
}
}
