# docs/specgen-mrdocs.sed                                           -*-sed-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# The second stage of the MrDocs filter, after docs/specgen-doxygen.sed.
#
# MrDocs has no ALIASES, so the wording elements Doxygen renders through them
# are rewritten here into the `\par` sections both understand. `\par` takes the
# rest of its line as the heading, so the element's text moves to a line of
# its own. That shifts line numbers, which is why this stage is not part of
# the Doxygen filter: `make mrdocs` runs MrDocs over a filtered copy of the
# headers, never the headers themselves.

s@^([[:space:]]*//!)[[:space:]]*\\constraints([[:space:]]+(.*))?$@\1 \\par Constraints:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\mandates([[:space:]]+(.*))?$@\1 \\par Mandates:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\expects([[:space:]]+(.*))?$@\1 \\par Preconditions:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\hardexpects([[:space:]]+(.*))?$@\1 \\par Hardened preconditions:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\effects([[:space:]]+(.*))?$@\1 \\par Effects:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\sync([[:space:]]+(.*))?$@\1 \\par Synchronization:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\ensures([[:space:]]+(.*))?$@\1 \\par Postconditions:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\throws([[:space:]]+(.*))?$@\1 \\par Throws:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\complexity([[:space:]]+(.*))?$@\1 \\par Complexity:\n\1 \3@
s@^([[:space:]]*//!)[[:space:]]*\\remarks([[:space:]]+(.*))?$@\1 \\par Remarks:\n\1 \3@
