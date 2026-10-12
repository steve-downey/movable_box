# `@seebelow noexcept`: the command cannot target a specifier, and silently drops its argument into the description

## Summary

`@seebelow` (`\seebelow`) flags the whole symbol as see-below.
Text after the command on the same line is not an argument.
It becomes a paragraph of the symbol's description, and MrDocs gives no diagnostic.

The C++ working draft often writes see-below for one part of a declaration only:

```cpp
constexpr optional(optional&& rhs) noexcept(see below);
constexpr explicit(see below) optional(U&& v);
```

MrDocs can already render the first shape.
A long `noexcept` operand is shown as `noexcept(/* see-below */)`, with an *Exception specification* section, once it passes `noexcept-see-below-limit`.
But a comment has no way to ask for that rendering, and the natural spelling, `@seebelow noexcept`, does something else without saying so.

## Environment

- MrDocs release 2026.9.29 (version 0.8.0), Linux x86_64 release tarball
- Built with LLVM 23.0.0git, build SHA 6a984f66cf770ee5808bc0f53614c522c68fa507

## Reproduction

`include/box.hpp`:

```cpp
namespace demo {

template <class T>
class box {
  public:
    /// Copy assignment.
    ///
    /// The exception specification is shown as `noexcept(see below)`
    /// in the generated specification; the condition is given in prose.
    ///
    /// @seebelow noexcept
    box& operator=(const box& rhs) noexcept(noexcept(T(rhs.value_)));

    /// Move assignment, documented without the command.
    box& operator=(box&& rhs) noexcept(noexcept(T(static_cast<T&&>(rhs.value_))));

  private:
    T value_;
};

} // namespace demo
```

`mrdocs.yml`:

```yaml
source-root: .
input:
  - include
includes:
  - include
file-patterns:
  - '*.hpp'
include-symbols:
  - 'demo::**'
multipage: false
generator: adoc
output: out
```

```sh
mrdocs mrdocs.yml
mrdocs mrdocs.yml --generator=xml --output=outxml
```

For comparison, run it a second time with the `@seebelow noexcept` line deleted.

## Observed

With the command, the run reports `see-below symbols: 1`, and the XML marks the copy assignment operator `<extraction>see-below</extraction>`.
No warning is emitted for the command or its argument.

The copy assignment operator's page renders its full exception specification:

```
operator=(box const& rhs) noexcept(noexcept(T(rhs.value_)));
```

The command's argument shows up as a paragraph of its own in the description:

```
=== Description
The exception specification is shown as `noexcept(see below)` in the
generated specification; the condition is given in prose.
noexcept
```

Without the command, the page is the same except for the stray `noexcept` paragraph.
So on a function the command changes the extraction mode and adds the stray text, but nothing the reader would recognize as see-below.

The move assignment operator, which has no command, is the one rendered with see-below, because its operand is longer than `noexcept-see-below-limit`:

```
operator=(box&& rhs) noexcept(/* see-below */);

=== Exception specification
This function is potentially-throwing unless `noexcept(T(static_cast<T &&>(rhs.value_)))`.
```

## Expected

1. `@seebelow noexcept` renders that function's exception specification as `noexcept(/* see-below */)`, as the length limit already does, whatever the operand's length.
   `@seebelow explicit` does the same for `explicit(...)`.
   The rest of the declaration renders as usual.
2. Text after a flag command that it does not accept as an argument produces a warning, rather than turning into description text.

## Why it matters

We generate standard wording from annotated C++ headers with [beman.specgen](https://github.com/steve-downey/specgen), and build API documentation from the same headers with MrDocs.
specgen uses `\seebelow noexcept` and `\seebelow explicit` with exactly the meaning in *Expected* above, matching the draft's `noexcept(@\seebelow@)` and `explicit(@\seebelow@)`.
A bare `\seebelow` there masks a whole return type, alias target, or variable initializer, which is close to MrDocs' own whole-symbol see-below.

Today we rewrite the headers before MrDocs reads them, blanking every `\seebelow` line, because MrDocs gives the command a different meaning with no way to turn it off.
Either of these would let both tools read the same comments:

- the targeted form above; or
- a configuration option to turn off, or rename, a built-in flag command such as `seebelow`, so a project can keep the spelling for another tool.
