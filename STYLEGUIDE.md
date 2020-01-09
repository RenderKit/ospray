OSPRay Style Guide
==================

This document discusses the project's expectations on C++ and ISPC
style. In general, the items discussed are meant to be a guide and not
be overly strict. However, deviations from this document should be
supported with technical reasoning.

This document is relatively simple and meant as a "quick overview" of
expecations for OSPRay. A more complete set of guidelines for all of C++
can be found in the [ISO C++ Core
Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md).

Documentation
-------------

In general, comments in code should describe "why" not "what". Code
documentation should be in terms of C++ style single-line comments or
multi-line C style comments. Avoid Doxygen-style code documentation,
this is deprecated.

When adding or updating public API definitions, update the corresponding
`doc/api.md` documentation as part of your merge request. This keeps our
documentation always current, and minimizes the need for audits at
release time. Markdown files (`.md`) should use a 72 column limit for
running text, exceptions are URLs or example code (i.e. preformatted
sections in backticks).

General Code Goals
------------------

In all code, the overall goal is to make interfaces "easy to use
correctly" and "difficult to use incorrectly". Some characteristics that
make code better are:

-   Keep reasoning as local as possible
-   Descriptive names
-   Short functions
-   Single functions/abstractions for repetitive concepts (less
    copy/paste)
-   Compile errors over runtime errors

We have a number of small, single-purpose abstractions in `ospcommon`
that aide in the above goals. It is expected for contributions to first
try and use those constructs before implementing their own.

Lastly, it is expected that code is formatted using the provided
`.clang-format` config found in the root of the repository, e.g. by
running `git-clang-format` (which also avoids changes which are only due
to formatting). Consider to pre-format very long strings using C++11 raw
string literals.

NOTE: Both C++ and ISPC files can be formatted with clang-format.

Naming conventions for public API
---------------------------------

-   Prefer enums over string parameters.
-   Parameter and object names are camelCase (e.g. `metallicPaint`)
-   Parameter names are singular, even if it is an array (e.g. `index`,
    `material`)
-   Data belonging together like the members of an array of structs,
    which are passed as struct of arrays, have a common prefix. For
    example `vertex.color` and `vertex.normals`
-   Switches (bools) name just what they switch, e.g. `shadows` (not
    `shadowsEnabled` nor `enableShadows`, nor `useShadows`)

C++ Usage Guidelines
--------------------

C++ is a big language, so the following are things that we prefer to see
in OSPRay:

### Memory Management, Object Lifetimes, and Pointers

-   For collections, prefer STL containers over C arrays, specifically
    `std::vector` and `ospcommon::containers::AlignedVector`.
-   Prefer smart pointers to manage memory over raw `new` and `delete`.
-   Prefer the usage of `std::unique_ptr` to manage heap objects, until
    memory lifetimes must be managed by more than one observer.
-   Prefer constructing smart pointers with `ospcommon::make_unique<>`
    and `std::make_shared<>` over using `new`.
-   Avoid globals and `std::shared_ptr` that cross translation units.
-   Prefer global scope variables to be marked `static`.
-   Prefer functions to access global data in another translation unit
    over marking them as `extern`.
-   Keep raw pointer usage local, either in a function or in a single
    cpp file.
-   Prefer the usage of references over pointers for cases where
    `nullptr` is not a valid value.
-   Prefer using `ospcommon::utility::OnScopeExit` to robustly cleanup
    non-trivial objects in a function.
-   Avoid constructing `static` data that requires initialization
    ordering with other `static` data.

### Designing Types (classes/structs)

-   Define empty default constructors with `= default;`, not with an
    empty `{}`.
-   Use the `virtual` keyword only to mark that child classes are
    expected to override, not that the function is overriding a parent
    method.
-   Always use the `override` keyword where appropriate.
-   Prefer to write `const` functions by default.
-   Prefer implementing inline member functions below the definition of
    the class or struct. This makes classes easier to read by avoiding
    unnecessary implementation details when reading the class interface.
-   Implement `begin()` and `end()` iterators for types which abstract
    collections.
-   Consider whether the type should be move-only (ex:
    `std::unique_ptr`) or copyable.
-   Prefer `= delete;` syntax to delete constructors over marking them
    `private`.
-   Prefer giving all applicable member values inlined default values.
-   Only populate values in constructor initialization if the value is
    different than the default or is being set by a constructor
    parameter.
-   Only mark methods as `virtual` if the class is intended to be in an
    inheritance hierarchy.
-   Always implement a `virtual` destructor, even if it's `= default;`.

### Loops, STL Algorithms, and Threading

-   Prefer the usage of an algorithm call over implementing a raw loop.
-   Prefer making multi-STL algorithm functions their own function with
    a name.
-   Prefer using `ospcommon::tasking` constructs instead of raw threads.
-   Prefer range-based for loops over indexing for loops where possible.
-   Avoid shared mutable state in parallel code (requires
    synchronization).
-   Capture type preference for lambda functions should be `[]` → `[&]`
    → `[=]`.

### Templates

-   Prefer to constrain template types to only relevant types using
    `static_assert()` and type traits.
-   Use `std::forward` when passing universal references (`T&&`) to
    other templates.
-   Only use SFINAE when necessary, and try to keep its usage as simple
    as possible.

ISPC Usage
----------

The following are concerns specific to ISPC:

-   Prefer using `uniform` variables. The point of ISPC is to have
    varying data, but uniform values are more efficient than replicated
    varying values.
-   Avoid writing unnecessary `uniform` and `varying` keywords.
    Examples:

    ``` {.cpp}
      varying float f; // 'varying' is redundant, variables are varying by default
      uniform float *p; // 'uniform' is redundant, pointers point to uniform by default
    ```

-   Prefer omitting `uniform`/`varying` qualifiers for structs. Only
    apply them if they need to be consistenly the same for both
    `uniform` and `varying` instances of the struct.

### OSPRay ISPCDevice Specific Design Choices

-   Prefer to use Embree C API bindings over ISPC where possible.
-   Prefer to throw exceptions when an object is found to be invalid
    during `commit()`.
-   Prefer setting ISPC-side values in a single `set()` call over many
    single value setting functions.
-   Minimize the number of alias names given to both object types and
    parameters.
