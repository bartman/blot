# this make file is a wrapper around CMake and provided for convience only.
# actual rules for building software and components should be in cmake scripts.

.PHONY: all config build clean distclean install test check smoke
.DEFAULT_GOAL := all

USER = $(shell id -u -n)
SUDO = $(shell which sudo)

DESTDIR    ?=
BUILDDIR    = build

Q = $(if ${V},,@)

all: build

# ---------------------------------------------------------------------------

BUILDNINJA  = ${BUILDDIR}/build.ninja
CMAKEFLAGS  = -G Ninja

DEFAULT_TYPE = Release
CURRENT_TYPE = $(shell sed -r -n -e 's/CMAKE_BUILD_TYPE:STRING=//p' ${BUILDDIR}/CMakeCache.txt 2>/dev/null)
TYPE ?= $(if ${CURRENT_TYPE},${CURRENT_TYPE},${DEFAULT_TYPE})
CMAKEFLAGS += -DCMAKE_BUILD_TYPE=${TYPE}

# prefer clang over gcc
ifneq ($(shell which clang),)
CMAKEFLAGS += -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
else
ifneq ($(shell which gcc),)
CMAKEFLAGS += -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
else
# otherwise use default
endif
endif

ifneq (${TYPE},${CURRENT_TYPE})
ifneq (${CURRENT_TYPE},)
$(info ### switching from ${CURRENT_TYPE} to ${TYPE})
endif
.PHONY: ${BUILDNINJA}
endif

config: ${BUILDNINJA}

${BUILDNINJA}: Makefile CMakeLists.txt
	@echo "### setting up cmake in ${BUILDDIR} for ${TYPE}"
	${Q}mkdir -p ${BUILDDIR}
	${Q}cmake -B${BUILDDIR} -S. ${CMAKEFLAGS}

build/compile_commands.json: config

COMPILE_COMMANDS = compile_commands.json
${COMPILE_COMMANDS}: build/compile_commands.json
	@echo "### linking compile_commands.json"
	${Q}ln -fs $< $@

build: ${BUILDNINJA} ${COMPILE_COMMANDS}
	@echo "### building in ${BUILDDIR}"
	${Q}ninja -C ${BUILDDIR}

clean:
	@echo "### cleaning in ${BUILDDIR}"
	${Q}ninja -C ${BUILDDIR} clean

distclean:
	@echo "### removing ${BUILDDIR}"
	${Q}rm -rf build

install: build
	@echo "### installing from ${BUILDDIR}"
	${Q}DESTDIR=$(abspath ${DESTDIR}) ninja -C ${BUILDDIR} install

test check: build
	@echo "### unit-testing from ${BUILDDIR}"
	${Q}cd ${BUILDDIR} && ctest --output-on-failure --output-junit test_report.xml

smoke: build
	@echo "### smoke-testing from ${BUILDDIR}"
	@echo there are no smoke tests yet

# ---------------------------------------------------------------------------

.PHONY: tags cscope cscope.files

TAG_DIRS = lib include cli test
tags: cscope ctags

cscope: cscope.out
cscope.out: cscope.files
	${Q}cscope -P`pwd` -b

cscope.files:
	${Q}find ${TAG_DIRS} -type f -name '*.[ch]' -o -name '*.[ch]pp' \
		| grep -v -e /rpm/ -e CVS -e SCCS > $@

ctags: cscope.files
	${Q}ctags --sort=yes -L $<

