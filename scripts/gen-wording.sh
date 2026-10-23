#!/usr/bin/env bash
# scripts/gen-wording.sh                                              -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Generates the [movable.box] wording fragments from the marked-up header.
#
# include/beman/movable_box/movable_box.hpp is the single source of the
# proposed subclause's synopsis and item wording: its class body is the
# synopsis, its `//!` docblocks are the wording, and its `\rSec3` markers place
# each item. beman.specgen renders that as mpark/wg21 Markdown fragments or
# one C++ draft LaTeX file. papers/movable_box.md splices in the Markdown;
# the minimal papers/movable_box_std.tex example inputs the LaTeX.
# The general subclause's prose stays in each paper: it is not about any one
# declaration.
#
# One header is one document, so this is a single `generate`; there is no
# second document for `--validate` to see. The synopsis sits outside every
# `\rSec3` and lands in the root fragment, named for the subclause it belongs
# in. `--new-root movable.box` marks the whole subclause as proposed, so its
# stable names are not looked up in the working draft.
#
# The MPark backend writes split fragments and an ordered manifest beside
# them. The manifest is the record the paper's include order is reconciled
# against, and is not a `.md` file the vendored MPark.WG21 Makefile would
# pass to pandoc as an input. The LaTeX example needs no assembly: it inputs
# specgen's single joined output.
#
# Any arguments after the output directory are passed to `specgen generate`;
# the top-level Makefile passes `--depfile` and `--dep-target` through them.
# Paths in them must be absolute, because specgen runs from the output
# directory.
#
# Set SPECGEN_BACKEND to `latex` for C++ draft LaTeX; the default is `mpark`.
#
# Usage: scripts/gen-wording.sh [output-directory [specgen-option...]]

set -o nounset
set -o errexit
set -o pipefail
IFS=$'\n\t'

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

UVX="${UVX:-uvx}"
SPECGEN_BACKEND="${SPECGEN_BACKEND:-mpark}"
OUT_DIR="${1:-$REPO_ROOT/papers/wording}"
shift $(( $# > 0 ? 1 : 0 ))

readonly HEADER="$REPO_ROOT/include/beman/movable_box/movable_box.hpp"
readonly SPECGEN_INDEX="https://test.pypi.org/simple"
# specgen is pinned because its output is checked in, and its version is part
# of what that output depends on. The Makefile cannot see which release uvx
# would resolve, but it does list this script as a prerequisite of the
# wording, so bumping the pin here is what makes `make papers` regenerate it.
readonly SPECGEN_VERSION="0.5.1"
readonly -a SPECGEN_CMD=(
    "$UVX"
    --index "$SPECGEN_INDEX"
    "beman-specgen==$SPECGEN_VERSION"
)

case "$SPECGEN_BACKEND" in
    mpark)
        readonly OUTPUT="movable_box.manifest"
        readonly -a BACKEND_ARGS=(
            --new-root movable.box
            --base-heading-level 4
            --split .
            --root movable.box.general
        )
        ;;
    latex)
        readonly OUTPUT="/dev/null"
        readonly -a BACKEND_ARGS=(
            --base-section-depth 3
            --output movable_box.tex
        )
        ;;
    *)
        echo "gen-wording: error: unknown SPECGEN_BACKEND '$SPECGEN_BACKEND'" >&2
        exit 1
        ;;
esac

# Flags are passed explicitly rather than through a compilation database,
# because a header is not a translation unit in one. config.hpp falls back to
# a non-modules build when the CMake-generated config header is absent.
# Point clang at the system GCC installation so an isolated uvx environment
# can discover the standard library without version-specific include paths.
readonly -a CLANG_ARGS=(
    -std=c++2c
    --gcc-toolchain=/usr
    "-I$REPO_ROOT/include"
)

if ! command -v "$UVX" > /dev/null 2>&1; then
    echo "gen-wording: error: '$UVX' not found; set UVX to its path" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# `generate` prints docblock findings on stderr and still exits 0 on one, so
# its stderr is captured and any finding fails the run.
FINDINGS="$(mktemp)"
trap 'rm -f "$FINDINGS"' EXIT

echo "gen-wording: movable_box.hpp -> $SPECGEN_BACKEND movable.box.*"
# Run from the output directory so every generated path is portable.
status=0
(
    cd "$OUT_DIR"
    "${SPECGEN_CMD[@]}" generate "$HEADER" \
        --no-compile-commands \
        --backend "$SPECGEN_BACKEND" \
        --validate \
        "${BACKEND_ARGS[@]}" \
        "$@" \
        -- "${CLANG_ARGS[@]}" \
        > "$OUTPUT" 2> "$FINDINGS"
) || status=$?

if [[ $status -ne 0 || -s "$FINDINGS" ]]; then
    cat "$FINDINGS" >&2
    echo "gen-wording: error: specgen reported findings; fix the header markup" >&2
    exit 1
fi

echo "gen-wording: wrote $OUT_DIR"
