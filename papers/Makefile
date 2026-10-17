# papers/Makefile                                       -*-Makefile-*-
include wg21/flat.mk

# The generated wording fragments are spliced in by filters/transclude.py, so
# the paper must rebuild when they change. The fragments themselves must NOT
# be prerequisites: base.mk passes every `%.md` prerequisite to pandoc as a
# further input file. Depend on the manifest written beside them instead;
# every regeneration rewrites it. The top-level `make papers` regenerates the
# wording first.
$(OUTDIR)/movable_box.html $(OUTDIR)/movable_box.latex $(OUTDIR)/movable_box.pdf: \
    wording/movable_box.manifest filters/transclude.py filters/bibliography-header.py

STD_LATEX_DIR := $(CURDIR)/wg21-latex
STD_LATEX_SUPPORT := $(STD_LATEX_DIR)/common.tex \
    $(wildcard $(STD_LATEX_DIR)/stdtex/*.tex)
STD_PAPER := $(OUTDIR)/movable_box_std.pdf

.PHONY: papers std-paper
papers: all std-paper
std-paper: $(STD_PAPER)

$(STD_PAPER): movable_box_std.tex wording/movable_box.tex $(STD_LATEX_SUPPORT) | $(OUTDIR)
	TEXINPUTS="$(STD_LATEX_DIR)//:" latexmk -pdflua -shell-escape \
	    -interaction=nonstopmode -halt-on-error -outdir=$(OUTDIR) $<

STD_PAPER_ARTIFACTS := $(addprefix $(OUTDIR)/movable_box_std., \
    aux fdb_latexmk fls idx ilg ind log out pdf toc)

.PHONY: clean-std-paper
clean-std-paper:
	rm -f $(STD_PAPER_ARTIFACTS) $(addprefix $(OUTDIR)/, \
	    std-gram.ext xrefdelta.glo xrefindex.glo)

clean: clean-std-paper
