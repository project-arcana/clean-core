# clean-core
clean-core (`cc`) is a clean and lean reimagining of the C++ standard library.

## Goals

* significantly faster to compile than `std`
* forward declaration for all public types
* no slower than `std`
* safer than `std`
* more modular header design (each type can be separately included)
* convenient interfacing with code using `std` types
* removal of unintuitive behavior (e.g. `vector<bool>` or `optional::operator bool`)
* mostly keeping naming scheme and intent of `std`
* better debugging support and performance
* no dependency on `exception`s

## Requirements / Dependencies

* a C++17 compiler
* a few `std` features (that are hard to otherwise implement)

## Notable Changes

Changes that were rarely used features that increased implementation cost immensely:

* no `allocator`s
* no custom deleters for `unique_ptr`

Error-prone and unintuitive or suprising features:

* no specialized `vector<bool>`
* no `operator bool` for `optional<T>`
* no `operator<` for `optional<T>`

Others:

* no strong `exception` support
* no iterator-pair library, only ranges
* no unreadable `_UglyCase` (leaking non-caps macros is a sin)
* traits types and values are not suffixed with `_t` or `_v`
* no `volatile` support
* no `unique_ptr<T[]>`

## New Features

* `span` (strided array view)
* `flat_` containers
* `inline_` types (no heap allocs)
* customizable low-impact `assert`
* optional bound-checked containers
* optional null checks for smart pointer
