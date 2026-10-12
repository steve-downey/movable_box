# Clang block commands MrDocs does not handle are dropped with their text, silently (`@remarks`, `@deprecated`, `@retval`, ...)

## Summary

A block command that Clang's comment parser knows, but MrDocs has no case for, is discarded together with its paragraph.
Nothing is reported.
`@remarks`, `@deprecated`, `@retval`, `@invariant`, `@sa` and `@todo` are among them.

This looks unintended: the text is in the comment, Clang has parsed it as a block, and it disappears.

## Environment

- MrDocs release 2026.9.29 (version 0.8.0), Linux x86_64 release tarball
- Built with LLVM 23.0.0git, build SHA 6a984f66cf770ee5808bc0f53614c522c68fa507

## Reproduction

`include/a.hpp`:

```cpp
namespace demo {

/// Brief.
///
/// @remarks This remark is lost.
///
/// This second paragraph survives.
int f();

/// Brief.
///
/// @deprecated Use `g2` instead.
int g();

/// Brief.
///
/// @retval 0 Success.
int h();

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

## Observed

None of "This remark is lost.", "Use `g2` instead." or "Success." appears in either output.
For `f`, only "This second paragraph survives." is kept: the `@remarks` paragraph is gone, and the paragraph after it is ordinary description.
The run reports no warning for any of the three.

Testing each of Clang's plain block commands the same way, with one function per command, gives:

| Text kept | Text dropped |
|---|---|
| `note`, `warning`, `see`, `pre`, `post`, `details` | `remark`, `remarks`, `attention`, `author`, `authors`, `bug`, `copyright`, `date`, `invariant`, `since`, `todo`, `version`, `sa`, `seealso`, `deprecated`, `discussion`, `test`, `arg`, `retval` |

(`brief`, `returns`, `param`, `tparam`, `throws` and `par` are handled and are not in the table.)

## Cause

In `src/mrdocs/AST/ExtractDocComment.cpp`, the block-command visitor's `default:` case handles `tip`, `important`, `caution` and the custom flag commands, then ends:

```cpp
// unsupported → ignore
return;
```

The paragraph attached to the command is never visited.

## Expected

The text is kept, even when MrDocs has no special rendering for the command.
Any of these would do, in order of preference:

1. Render the commands that have an obvious meaning: `@deprecated`, `@remarks`, `@sa`/`@seealso`, `@retval`, `@invariant`, `@since`, `@todo`.
2. For the rest, keep the paragraph as description, as is already done for the paragraph after a flag command, perhaps labelled with the command's name.
3. At minimum, warn that a command and its text were ignored, with the source location.

Some of these commands carry information users rely on, `@deprecated` and `@retval` most of all, and losing it without a diagnostic makes the gap hard to notice.
