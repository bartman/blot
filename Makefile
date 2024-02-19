# this make file is a wrapper around CMake and provided for convience only.
# actual rules for building software and components should be in cmake scripts.

.PHONY: all config build clean distclean install check
.DEFAULT_GOAL := all

USER = $(shell id -u -n)
SUDO = $(shell which sudo)

DESTDIR    ?=
BUILDDIR    = build

all: build

# ---------------------------------------------------------------------------

BUILDNINJA  = ${BUILDDIR}/build.ninja
CMAKEFLAGS  = -G Ninja
CMAKEFLAGS += -DCMAKE_BUILD_TYPE=Debug

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

config: ${BUILDNINJA}

${BUILDNINJA}: Makefile CMakeLists.txt
	mkdir -p ${BUILDDIR}
	cmake -B${BUILDDIR} -S. ${CMAKEFLAGS} 

build/compile_commands.json: config

COMPILE_COMMANDS = compile_commands.json
${COMPILE_COMMANDS}: build/compile_commands.json
	ln -fs $< $@

build: ${BUILDNINJA} ${COMPILE_COMMANDS}
	ninja -C ${BUILDDIR}

clean:
	ninja -C ${BUILDDIR} clean

distclean:
	rm -rf build

install: build
	DESTDIR=$(abspath ${DESTDIR}) ninja -C ${BUILDDIR} install

check: build
	cd ${BUILDDIR} && ctest

# ---------------------------------------------------------------------------

.PHONY: tags cscope cscope.files

TAG_DIRS = lib include cli test
tags: cscope ctags

cscope: cscope.out
cscope.out: cscope.files
	cscope -P`pwd` -b

cscope.files:
	find ${TAG_DIRS} -type f -name '*.[ch]' -o -name '*.[ch]pp' \
		| grep -v -e /rpm/ -e CVS -e SCCS > $@

ctags: cscope.files
	ctags --sort=yes -L $<

