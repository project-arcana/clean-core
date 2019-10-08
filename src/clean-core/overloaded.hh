#pragma once

namespace cc
{
/**
 * Overloading lambdas and other functors:
 *
 *   cc::overloaded(
 *      [...](...) { ... },
 *      [...](...) { ... },
 *      ...
 *   );
 */
template <class... Fs>
struct overloaded : Fs...
{
    overloaded(Fs... fs) : Fs(fs)... {}
    using Fs::operator()...;
};
}
