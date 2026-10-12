#!/usr/bin/env bash
# scripts/gen-wording.sh                                              -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Generates the [movable.box] wording fragments from the marked-up header.
#
# include/beman/movable_box/movable_box.hpp is the single source of the
# proposed subclause's synopsis and item wording: its class body is the
# synopsis, its `//!` docblocks are the wording, and its `\rSec3` markers place
# each item. beman.specgen renders that as mpark/wg21 markdown, one fragment
# per subclause, which papers/movable_box.md splices in through
# papers/filters/transclude.py. The general subclause's prose stays in the
# paper: it is not about any one declaration.
#
# One header is one document, so this is a single `generate`; there is no
# second document for `--validate` to see. The synopsis sits outside every
# `\rSec3` and lands in the root fragment, named for the subclause it belongs
# in. `--new-root movable.box` marks the whole subclause as proposed, so its
# stable names are not looked up in the working draft.
#
# `--split` never deletes files left by an earlier run, so the ordered
# manifest is written beside the fragments and is the record the paper's
# include order is reconciled against. It is also what the paper build
# depends on: it is rewritten on every run, and it is not a `.md` file, which
# the vendored MPark.WG21 Makefile would pass to pandoc as an input.
#
# Any arguments after the output directory are passed to `specgen generate`;
# the top-level Makefile passes `--depfile` and `--dep-target` through them.
# Paths in them must be absolute, because specgen runs from the output
# directory.
#
# Usage: scripts/gen-wording.sh [output-directory [specgen-option...]]

set -o nounset
set -o errexit
set -o pipefail
IFS=$'\n\t'

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

SPECGEN="${SPECGEN:-specgen}"
OUT_DIR="${1:-$REPO_ROOT/papers/wording}"
shift $(( $# > 0 ? 1 : 0 ))

readonly HEADER="$REPO_ROOT/include/beman/movable_box/movable_box.hpp"
readonly MANIFEST="movable_box.manifest"

# Flags are passed explicitly rather than through a compilation database,
# because a header is not a translation unit in one. config.hpp falls back to
# a non-modules build when the CMake-generated config header is absent, so the
# source include directory is all the parse needs.
readonly -a CLANG_ARGS=(
    -std=c++2c
    "-I$REPO_ROOT/include"
)

if ! command -v "$SPECGEN" > /dev/null 2>&1; then
    echo "gen-wording: error: '$SPECGEN' not found; set SPECGEN to its path" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# `generate` prints docblock findings on stderr and still exits 0 on one, so
# its stderr is captured and any finding fails the run.
FINDINGS="$(mktemp)"
trap 'rm -f "$FINDINGS"' EXIT

echo "gen-wording: movable_box.hpp -> movable.box.*"
# Run from the output directory with a relative `--split` so the manifest
# records `<stable name>.md` rather than a path particular to this machine.
status=0
(
    cd "$OUT_DIR"
    "$SPECGEN" generate "$HEADER" \
        --no-compile-commands \
        --backend mpark \
        --validate \
        --new-root movable.box \
        --base-heading-level 4 \
        --split . \
        --root movable.box.general \
        "$@" \
        -- "${CLANG_ARGS[@]}" \
        > "$MANIFEST" 2> "$FINDINGS"
) || status=$?

if [[ $status -ne 0 || -s "$FINDINGS" ]]; then
    cat "$FINDINGS" >&2
    echo "gen-wording: error: specgen reported findings; fix the header markup" >&2
    exit 1
fi

echo "gen-wording: wrote $OUT_DIR"
