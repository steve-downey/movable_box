#!/usr/bin/env python3

# papers/filters/bibliography-header.py                            -*-python-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Lets `wg21.py` run a second time.
#
# `wg21.py`'s `header` action does
#
#     if elem.identifier == 'bibliography':
#         elem.classes.remove('unnumbered')
#
# unconditionally. defaults.yaml runs `wg21.py` twice -- once in doc.yaml's
# chain and again after filters/transclude.py, so that spliced wording
# fragments are processed -- and the first pass has already removed the
# class, so the second raises
#
#     ValueError: list.remove(x): x not in list
#
# and the build aborts. This filter runs between the two passes and restores
# the class the second one expects to remove. The end state is the one
# `wg21.py` intends: a numbered references header.

import panflute as pf


def action(element, doc):
    if not isinstance(element, pf.Header):
        return None
    if element.identifier == "bibliography" and "unnumbered" not in element.classes:
        element.classes.append("unnumbered")
    return None


def main(doc=None):
    return pf.run_filter(action, doc=doc)


if __name__ == "__main__":
    main()
