# Unknown commands become plain text with the backslash removed, and there is no way to declare custom ones

## Summary

A command that neither Clang nor MrDocs knows, such as `\effects`, is treated as text.
The backslash is dropped, the command name stays in the output as a word, and consecutive commands run together into one paragraph.
No diagnostic is emitted.

This is mostly harmless: nothing is lost, the output just reads oddly.
The request is for a warning, and for a way to declare a project's own commands.

## Environment

- MrDocs release 2026.9.29 (version 0.8.0), Linux x86_64 release tarball
- Built with LLVM 23.0.0git, build SHA 6a984f66cf770ee5808bc0f53614c522c68fa507

## Reproduction

`include/a.hpp`:

```cpp
namespace demo {

/// \effects One-line effects.
int f1(int x);

/// \constraints `x` is positive.
/// \effects Effects text.
int f2(int x);

/// \effects Intro. Otherwise:
/// \item First item.
/// \item Second item.
int f3(int x);

/// \constraints-in-decl
/// \effects Effects text.
int f4(int x);

/// \omit
int f5(int x);

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
```

## Observed

Each function's documentation is a single brief:

| Function | Brief |
|---|---|
| `f1` | `effects One-line effects.` |
| `f2` | ``constraints `x` is positive. effects Effects text.`` |
| `f3` | `effects Intro. Otherwise: item First item. item Second item.` |
| `f4` | `constraints-in-decl effects Effects text.` |
| `f5` | `omit` |

The run reports only the usual missing-parameter and missing-return-value notices.
Adding `-fcomment-block-commands=effects,constraints` to the compilation database MrDocs reads makes no difference.

## Expected

1. A warning for an unknown command, with its location, as Clang gives with `-Wdocumentation-unknown-command`.
2. A configuration option to declare a project's commands, along the lines of Doxygen's `ALIASES`, in two kinds:
   - a **labelled block**: `effects: "Effects"` renders `\effects text` as a section headed *Effects*, the way `\par Effects:` does today;
   - an **ignored command**: `omit`, `merge`, `also` are markers meant for another tool, and are removed along with nothing else.

   Hyphenated names (`constraints-in-decl`) need to work; Clang's lexer stops a command name at the hyphen.

## Why it matters

The C++ standard library's wording is written with a fixed set of elements: *Constraints*, *Mandates*, *Preconditions*, *Effects*, *Synchronization*, *Postconditions*, *Returns*, *Throws*, *Complexity*, *Remarks*.
We write those as `\constraints`, `\effects` and so on in header comments, and generate standard wording from them with [beman.specgen](https://github.com/steve-downey/specgen).
Doxygen renders the same comments through `ALIASES`, such as `effects=\par Effects:^^`.
For MrDocs we rewrite each element into `\par` with `sed` and run MrDocs over the rewritten copy of the headers.
A declared-command option would remove that step.
