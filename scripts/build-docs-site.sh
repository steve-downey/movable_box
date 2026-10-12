#!/usr/bin/env bash
# scripts/build-docs-site.sh                                          -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Builds both API references and assembles them into one static site, the
# tree that is published to the gh-pages branch:
#
#   index.html   docs/site-index.html, linking the two
#   doxygen/     the Doxygen HTML (docs/html/)
#   mrdocs/      the MrDocs HTML (docs/adoc/, without the AsciiDoc sources)
#   .nojekyll    so GitHub Pages serves files whose names start with `_`
#
# A repository gets one gh-pages branch, so the two references are siblings
# under it rather than separate deployments.
#
# Needs doxygen (1.15 or later), mrdocs and asciidoctor; see
# scripts/build-mrdocs.sh. DOXYGEN names doxygen if it is not on PATH.
#
# Usage: scripts/build-docs-site.sh [output-directory]

set -o nounset
set -o errexit
set -o pipefail

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

DOXYGEN="${DOXYGEN:-doxygen}"
SITE="${1:-$REPO_ROOT/.build/site}"

cd "$REPO_ROOT"

rm -rf docs/html
"$DOXYGEN" docs/Doxyfile
scripts/build-mrdocs.sh

rm -rf "$SITE"
mkdir -p "$SITE/mrdocs"
cp docs/site-index.html "$SITE/index.html"
cp -R docs/html "$SITE/doxygen"
while IFS= read -r page; do
    mkdir -p "$SITE/mrdocs/$(dirname "$page")"
    cp "docs/adoc/$page" "$SITE/mrdocs/$page"
done < <(cd docs/adoc && find . -name '*.html')
touch "$SITE/.nojekyll"

echo "build-docs-site: wrote $SITE"
