# Documentation

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

`beman.movable_box` publishes two API references, built from the same header, to <https://bemanproject.github.io/movable_box/>:

- [Doxygen](https://bemanproject.github.io/movable_box/doxygen/), with [overview.md](overview.md) as its main page and the [example](../examples/movable_box.cpp);
- [MrDocs](https://bemanproject.github.io/movable_box/mrdocs/), one page per declaration.

The site's root page, [site-index.html](site-index.html), links the two.

## One header, three readers

[include/beman/movable_box/movable_box.hpp](../include/beman/movable_box/movable_box.hpp) carries two kinds of documentation comment:

- `///` comments are API documentation, read by Doxygen and MrDocs only.
- `//!` comments are [beman.specgen](https://github.com/steve-downey/specgen) markup: the proposed standard wording, from which `make papers` generates [the paper's](../papers/movable_box.md) wording.

Doxygen and MrDocs read `//!` comments too, so the wording appears in both references.
Neither tool knows specgen's commands, so each is taught them differently:

- **Doxygen.** `ALIASES` in [Doxyfile](Doxyfile) render the wording elements (`\effects`, `\constraints`, `\expects`, ...) as labelled paragraphs, the way the standard sets them.
  [specgen-doxygen.sed](specgen-doxygen.sed), the `INPUT_FILTER`, blanks the specgen directives (`\merge`, `\also`, `\omit`, `\seebelow`, `\constraints-in-decl`), which steer the wording and say nothing to a reader, and turns `\item` into Markdown list items.
  Every edit keeps the line, so Doxygen's line numbers stay the source's.
- **MrDocs.** It has no aliases and no input filter, so [scripts/build-mrdocs.sh](../scripts/build-mrdocs.sh) runs it over a copy of `include/` in `.build/mrdocs/`.
  The copy goes through the same directive filter, then [specgen-mrdocs.sed](specgen-mrdocs.sed), which rewrites each wording element as the `\par` section MrDocs understands.
  [mrdocs.yml](mrdocs.yml) points at the copy.

## Building locally

| Command | Builds | Needs |
|---|---|---|
| `make docs` | `docs/html/` | Doxygen 1.15 or later |
| `make mrdocs` | `docs/adoc/`, AsciiDoc and HTML | MrDocs 2026.9.29 or later, Asciidoctor |
| `make docs-site` | `.build/site/`, the tree that is published | all of the above |

All three outputs are ignored by git.
`MRDOCS`, `ASCIIDOCTOR` and `DOXYGEN` name the tools if they are not on `PATH`.

## Publishing

[docs-gh-pages.yml](../.github/workflows/docs-gh-pages.yml) runs on every push to `main`.
It installs pinned, checksum-verified releases of Doxygen and MrDocs, runs [scripts/build-docs-site.sh](../scripts/build-docs-site.sh), the same script as `make docs-site`, and pushes the result to the `gh-pages` branch.

A repository has one `gh-pages` branch, so the two references are sibling directories in one tree and deploy together:

```text
index.html    site-index.html
doxygen/      Doxygen HTML
mrdocs/       MrDocs HTML
.nojekyll     so GitHub Pages serves files whose names start with `_`
```

GitHub Pages must be set to serve the `gh-pages` branch from its root.

To move to newer Doxygen or MrDocs releases, change the version and SHA-256 pair at the top of the workflow.

## Other files

- [header.html](header.html), [doxygen-awesome.css](doxygen-awesome.css) and [doxygen-awesome-darkmode-toggle.js](doxygen-awesome-darkmode-toggle.js) are the Doxygen theme.
- [examples.dox](examples.dox) lists the examples Doxygen documents.
- [debug-ci.md](debug-ci.md) describes running the CI workflows locally with `act`.
