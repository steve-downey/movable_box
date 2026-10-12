#!/usr/bin/env bash
# scripts/build-mrdocs.sh                                             -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Builds the MrDocs reference into docs/adoc/, as AsciiDoc and as HTML.
#
# The headers' `//!` comments are beman.specgen markup: the proposed standard
# wording. MrDocs reads them as documentation but knows none of their
# commands, and has no aliases or input filter to teach it, so it runs over a
# filtered copy of include/ in .build/mrdocs/ instead of the headers
# themselves. docs/specgen-doxygen.sed blanks the specgen directives and turns
# \item into list items; docs/specgen-mrdocs.sed rewrites each wording element
# into the `\par` section MrDocs understands. docs/mrdocs.yml points at the
# copy.
#
# Needs mrdocs (2026.9.29 or later) and asciidoctor on PATH, or MRDOCS and
# ASCIIDOCTOR naming them.
#
# Usage: scripts/build-mrdocs.sh

set -o nounset
set -o errexit
set -o pipefail

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

MRDOCS="${MRDOCS:-mrdocs}"
ASCIIDOCTOR="${ASCIIDOCTOR:-asciidoctor}"

readonly FILTERED="$REPO_ROOT/.build/mrdocs"
readonly OUTPUT="$REPO_ROOT/docs/adoc"

cd "$REPO_ROOT"
rm -rf "$FILTERED" "$OUTPUT"

while IFS= read -r header; do
    mkdir -p "$FILTERED/$(dirname "$header")"
    sed -E -f docs/specgen-doxygen.sed -f docs/specgen-mrdocs.sed "$header" > "$FILTERED/$header"
done < <(find include -name '*.hpp')

# MrDocs colours its output even when it is not a terminal.
(cd docs && NO_COLOR=1 "$MRDOCS" mrdocs.yml) 2>&1 | sed 's/\x1b\[[0-9;]*m//g'

find "$OUTPUT" -name '*.adoc' -print0 | xargs -0 "$ASCIIDOCTOR"
