# papers/Makefile                                       -*-Makefile-*-
include wg21/flat.mk

# The generated wording fragments are spliced in by filters/transclude.py, so
# the paper must rebuild when they change. The fragments themselves must NOT
# be prerequisites: base.mk passes every `%.md` prerequisite to pandoc as a
# further input file. Depend on the stamp `make wording` writes instead.
$(OUTDIR)/movable_box.html $(OUTDIR)/movable_box.latex $(OUTDIR)/movable_box.pdf: \
    wording/.stamp filters/transclude.py filters/bibliography-header.py
