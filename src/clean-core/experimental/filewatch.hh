#pragma once

#include <atomic>

#include <clean-core/string_view.hh>
#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

namespace cc
{
/**
 * Watch files on disk for changes
 *
 * Usage:
 *
 *     auto watch = cc::filewatch::create(path);
 *
 *     // time passes...
 *
 *     if (watch.has_changed())
 *     {
 *         // reload resource..
 *          watch.set_unchanged();
 *     }
 *
 * NOTE:
 *   this class is move-only
 */
struct filewatch
{
public:
    /// returns true iff this watches a file
    bool is_valid() const { return _state != nullptr; }

    /// returns true iff the watch is valid and the watched file has changed
    bool has_changed() const;

    /// clears the "has_changed" status
    void set_unchanged();

    /// creates a filewatch for a specific path
    static filewatch create(cc::string_view filename);

    filewatch();
    filewatch(filewatch&&);
    filewatch& operator=(filewatch&&) noexcept;
    filewatch(filewatch const&) = delete;
    filewatch& operator=(filewatch const&) = delete;
    ~filewatch();

private:
    struct state;
    cc::unique_ptr<state> _state;
};
}
