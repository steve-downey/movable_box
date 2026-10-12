# MrDocs issue drafts

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

Unfiled issue drafts for [MrDocs](https://github.com/cppalliance/mrdocs), found while wiring MrDocs into this repository's documentation (see [docs/README.md](../docs/README.md)).
Each draft is self-contained; its directory holds the reproduction (`mrdocs.yml` and `include/`), checked against MrDocs release 2026.9.29 (0.8.0).

- [seebelow-specifier.md](seebelow-specifier.md): `@seebelow noexcept` cannot target a specifier, and drops its argument into the description.
- [unhandled-block-commands.md](unhandled-block-commands.md): Clang block commands MrDocs does not handle (`@remarks`, `@deprecated`, `@retval`, ...) are dropped with their text, silently.
- [unknown-commands.md](unknown-commands.md): unknown commands become plain text, and there is no way to declare custom ones.

This branch is a holding place, not for merging.
