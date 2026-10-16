#! /usr/bin/make -f
# Makefile                                                       -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

NO_COLOR=1

INSTALL_PREFIX?=.install/
BUILD_DIR?=.build
DEST?=$(INSTALL_PREFIX)
CMAKE_FLAGS?=


PYEXECPATH ?= $(shell which python3.13 || which python3.12 || which python3.11 || which python3.10 || which python3.9 || which python3.8 || which python3)
PYTHON ?= $(notdir $(PYEXECPATH))
VENV := .venv
UV := $(shell command -v uv 2> /dev/null)
ACTIVATE := $(UV) run
PYEXEC := $(UV) run python
MARKER=.initialized.venv.stamp

PRE_COMMIT := $(UV) run pre-commit

TARGETS := test clean all ctest

export

.update-submodules:
	git submodule update --init --recursive
	touch .update-submodules

.gitmodules: .update-submodules

CONFIG?=Asan

export

ifeq ($(strip $(TOOLCHAIN)),)
	_build_name?=build-system/
	_build_dir?=.build/
	_local_toolchain?=$(CURDIR)/cmake/toolchain.cmake
else
	_build_name?=build-$(TOOLCHAIN)
	_build_dir?=.build/
	_local_toolchain?=$(CURDIR)/cmake/$(TOOLCHAIN)-toolchain.cmake
endif

_configuration_types?="RelWithDebInfo;Debug;Tsan;Asan;Gcov"

_build_path?=$(_build_dir)/$(_build_name)
_build_path:=$(subst //,/,$(_build_path))
_build_path:=$(patsubst %/,%,$(_build_path))

VCPKG ?= $(shell command -v vcpkg 2> /dev/null)

ifeq ($(VCPKG),)
	_cmake_top_level?="./cmake/use-fetch-content.cmake"
	_toolchain:=$(_local_toolchain)
	# To use a local git mirror for faster builds, uncomment and adjust:
	# _args=-DBEMANINFRA_googletest_REPO=file:///path/to/local/googletest.git
	_args=
else
	_vcpkg_toolchain:=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
	_cmake_top_level?=$(_vcpkg_toolchain)
	export PROJECT_VCPKG_TOOLCHAIN=$(_local_toolchain)
	_toolchain:=$(_local_toolchain)
	_args=-DVCPKG_OVERLAY_TRIPLETS=$(CURDIR)/cmake -DVCPKG_TARGET_TRIPLET=x64-linux-custom
	# for debugging add 	-DVCPKG_INSTALL_OPTIONS="--debug"
endif

CMAKE ?= $(UV) run cmake
CTEST ?= $(UV) run ctest

define run_cmake =
	$(CMAKE) \
	-G "Ninja Multi-Config" \
	-DCMAKE_CONFIGURATION_TYPES=$(_configuration_types) \
	-DCMAKE_INSTALL_PREFIX=$(abspath $(INSTALL_PREFIX)) \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	-DCMAKE_PREFIX_PATH=$(CURDIR)/infra/cmake \
	-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=$(_cmake_top_level) \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_TOOLCHAIN_FILE=$(_toolchain) \
    $(_args) \
	$(_cmake_args) \
	$(CURDIR)
endef

default: test
.PHONY: default

$(_build_path):
	mkdir -p $(_build_path)

$(_build_path)/CMakeCache.txt: | $(_build_path) .gitmodules $(VENV)
	cd $(_build_path) && $(run_cmake)

$(_build_path)/compile_commands.json : $(_build_path)/CMakeCache.txt

.PHONY: compile_commands.json
compile_commands.json: $(_build_path)/compile_commands.json
compile_commands.json:
	if [ "$(shell readlink compile_commands.json)" != "$(_build_path)/compile_commands.json" ] ; then \
		ln -sf $(_build_path)/compile_commands.json ; \
	fi

TARGET:=all
.PHONY: TARGET

.PHONY: compile
compile: $(_build_path)/CMakeCache.txt
compile: compile_commands.json
compile:  ## Compile the project
	$(CMAKE) --build $(_build_path)  --config $(CONFIG) --target all -- -k 0

.PHONY: compile-headers
compile-headers: $(_build_path)/CMakeCache.txt ## Compile the headers
	 $(CMAKE) --build $(_build_path)  --config $(CONFIG) --target all_verify_interface_header_sets -- -k 0

.PHONY: install
install: $(_build_path)/CMakeCache.txt compile ## Install the project
	$(CMAKE) --install $(_build_path) --config $(CONFIG) --component movable_box_Development --verbose

.PHONY: clean-install
clean-install:
	-rm -rf .install

.PHONY: realclean
realclean: clean-install

.PHONY: ctest
ctest: $(_build_path)/CMakeCache.txt ## Run CTest on current build
	$(CTEST) --test-dir $(_build_path) --output-on-failure -C $(CONFIG)

.PHONY: ctest_
ctest_ : compile
	$(CTEST) --test-dir $(_build_path) --output-on-failure -C $(CONFIG)

.PHONY: test
test: ctest_ ## Rebuild and run tests

.PHONY: cmake
cmake: |  $(_build_path)
	cd $(_build_path) && ${run_cmake}

.PHONY: clean
clean: $(_build_path)/CMakeCache.txt ## Clean the build artifacts
	$(CMAKE) --build $(_build_path)  --config $(CONFIG) --target clean

.PHONY: realclean
realclean: ## Delete the build directory
	rm -rf $(_build_path)

# ---- Generated [movable.box] wording ---------------------------------------
#
# The proposed wording for [movable.box] is generated from the marked-up
# header by beman.specgen; see scripts/gen-wording.sh. The generated files
# are checked in, so the papers build without specgen.

UVX ?= uvx

WORDING_DIR := papers/wording
WORDING_MANIFEST := $(WORDING_DIR)/movable_box.manifest
LATEX_WORDING := $(WORDING_DIR)/movable_box.tex
# The fragments, read from the manifest the last run wrote, so a new `\rSec3`
# in the header needs no edit here.
WORDING_FRAGMENTS := $(WORDING_MANIFEST) \
    $(patsubst ./%,$(WORDING_DIR)/%,$(file < $(WORDING_MANIFEST)))
# What specgen read to produce the fragments, the header and everything it
# includes. Absent until the first run, hence $(wildcard) below.
WORDING_DEPFILE := $(BUILD_DIR)/wording.d
LATEX_WORDING_DEPFILE := $(BUILD_DIR)/wording-latex.d

# A grouped target (GNU Make 4.3+): one run writes all of these, and make must
# not run it once per fragment. Without specgen the checked-in fragments are
# used as they are, with a warning, so the paper still builds; they are not
# touched, so the warning repeats until specgen regenerates them.
$(WORDING_FRAGMENTS) &: include/beman/movable_box/movable_box.hpp scripts/gen-wording.sh
	@if command -v $(UVX) > /dev/null 2>&1; then \
	    mkdir -p $(dir $(WORDING_DEPFILE)) && \
	    scripts/gen-wording.sh $(CURDIR)/$(WORDING_DIR) \
	        --depfile $(CURDIR)/$(WORDING_DEPFILE) \
	        $(addprefix --dep-target ,$(WORDING_FRAGMENTS)); \
	else \
	    echo "warning: '$(UVX)' not found; using the checked-in wording" >&2; \
	fi

$(LATEX_WORDING): include/beman/movable_box/movable_box.hpp scripts/gen-wording.sh
	@if command -v $(UVX) > /dev/null 2>&1; then \
	    mkdir -p $(dir $(LATEX_WORDING_DEPFILE)) && \
	    SPECGEN_BACKEND=latex scripts/gen-wording.sh $(CURDIR)/$(WORDING_DIR) \
	        --depfile $(CURDIR)/$(LATEX_WORDING_DEPFILE) \
	        --dep-target $(LATEX_WORDING); \
	else \
	    echo "warning: '$(UVX)' not found; using the checked-in LaTeX wording" >&2; \
	fi

-include $(wildcard $(WORDING_DEPFILE))
-include $(wildcard $(LATEX_WORDING_DEPFILE))

.PHONY: wording
wording: $(WORDING_FRAGMENTS) $(LATEX_WORDING) ## Regenerate both forms of the [movable.box] wording

.PHONY: wording-check
wording-check: ## Check the checked-in wording matches what the header generates
	rm -rf $(BUILD_DIR)/wording-check
	scripts/gen-wording.sh $(CURDIR)/$(BUILD_DIR)/wording-check
	SPECGEN_BACKEND=latex scripts/gen-wording.sh $(CURDIR)/$(BUILD_DIR)/wording-check
	diff -r $(WORDING_DIR) $(BUILD_DIR)/wording-check

.PHONY: clean-wording
clean-wording:
	rm -rf $(BUILD_DIR)/wording-check
	rm -f $(WORDING_DEPFILE) $(LATEX_WORDING_DEPFILE)

clean: clean-wording

.PHONY: papers
papers: wording ## Regenerate the wording if needed, then rebuild the paper(s)
	$(MAKE) -C papers papers

.PHONY: clean-papers
clean-papers:
	$(MAKE) -C papers clean

clean: clean-papers

.PHONY: env
env:
	$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))

.DEFAULT: $(_build_path)/CMakeCache.txt ## Other targets passed through to cmake
	$(CMAKE) --build $(_build_path)  --config $(CONFIG) --target $@ -- -k 0

.PHONY: all
all: compile


.PHONY: venv
venv: ## Create python virtual env
venv: $(VENV)/$(MARKER)

.PHONY: clean-venv
clean-venv:
	-rm -rf $(VENV)

realclean: clean-venv

.PHONY: show-venv
show-venv: venv
show-venv: ## Debugging target - show venv details
	$(PYEXEC) -c "import sys; print('Python ' + sys.version.replace('\n',''))"
	@echo venv: $(VENV)

uv.lock: pyproject.toml
	$(UV) lock

$(VENV):
	$(UV) venv --python $(PYTHON)

$(VENV)/$(MARKER): uv.lock | $(VENV)
	$(UV) sync
	touch $(VENV)/$(MARKER)

.PHONY: dev-shell
dev-shell: venv
dev-shell: ## Shell with the venv activated
	$(ACTIVATE) $(notdir $(SHELL))

.PHONY: bash zsh
bash zsh: venv
bash zsh: ## Run bash or zsh with the venv activated
	$(ACTIVATE) $@

.PHONY: lint
lint: venv
lint: ## Run all configured tools in pre-commit
	$(PRE_COMMIT) run -a

.PHONY: lint-manual
lint-manual: venv
lint-manual: ## Run all manual tools in pre-commit
	$(PRE_COMMIT) run --hook-stage manual -a

.PHONY: coverage
coverage: ## Build and run the tests with the GCOV profile and process the results
coverage: venv $(_build_path)/CMakeCache.txt
	$(CMAKE) --build $(_build_path) --config Gcov
	$(ACTIVATE) ctest --build-config Gcov --output-on-failure --test-dir $(_build_path)
	$(CMAKE) --build $(_build_path) --config Gcov --target process_coverage

.PHONY: view-coverage
view-coverage: ## View the coverage report
	sensible-browser $(_build_path)/coverage/coverage.html

.PHONY: docs
docs: ## Build the docs with Doxygen
	doxygen docs/Doxyfile

.PHONY: mrdocs
mrdocs: ## Build the docs with MrDocs
	scripts/build-mrdocs.sh

.PHONY: docs-site
docs-site: ## Build both references into the site published to gh-pages, in .build/site
	scripts/build-docs-site.sh $(CURDIR)/$(BUILD_DIR)/site

.PHONY: testinstall
testinstall: install
testinstall: ## Test the installed package
	$(CMAKE) -S installtest -B installtest/.build \
		-DCMAKE_PREFIX_PATH=$(abspath $(INSTALL_PREFIX))
	$(CMAKE) --build installtest/.build
	$(CTEST) --test-dir installtest/.build --output-on-failure

.PHONY: clean-testinstall
clean-testinstall:
	-rm -rf installtest/.build

realclean: clean-testinstall

# Git subtree management for infra/
INFRA_REMOTE ?= https://github.com/bemanproject/infra.git
INFRA_BRANCH ?= main

.PHONY: subtree-split
subtree-split: ## Split infra changes into a branch for upstreaming as a PR
	git subtree split --prefix=infra -b infra-upstream

.PHONY: subtree-add
subtree-add: ## Initial import of infra as git subtree (squash)
	git subtree add --squash --prefix=infra $(INFRA_REMOTE) $(INFRA_BRANCH)

ifeq ($(UV),)
define uv_error_message

'uv' command not found.
Please install uv or set the UV variable to the path of the uv binary.
endef

$(error "$(uv_error_message)")
endif

# Help target
.PHONY: help
help: ## Show this help.
	@awk 'BEGIN {FS = ":.*?## "} /^[.a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'  $(MAKEFILE_LIST) | sort
