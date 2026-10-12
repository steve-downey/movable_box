# docs/specgen-doxygen.sed                                          -*-sed-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Doxygen input filter for headers carrying beman.specgen markup.
#
# specgen reads `//!` docblocks as standard wording; Doxygen reads them too, as
# Qt-style comments. The wording elements (\effects, \constraints, ...) become
# draft-style paragraphs through ALIASES in docs/Doxyfile. What is left here is
# what an alias cannot do: blank the specgen directives, which steer the
# wording and say nothing to a reader, and turn \item into a Markdown list
# item. Every edit keeps the line, so Doxygen's line numbers stay the source's.

s@^([[:space:]]*//!)[[:space:]]*\\(merge|omit|also|expos|constraints-in-decl)([[:space:](].*)?$@\1@
s@^([[:space:]]*//!)[[:space:]]*\\seebelow([[:space:]].*)?$@\1@
s@^([[:space:]]*//!)[[:space:]]*\\item[[:space:]]@\1 - @
