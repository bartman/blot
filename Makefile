# this make file is a wrapper around CMake and provided for convience only.
# actual rules for building software and components should be in cmake scripts.

.PHONY: all config build clean distclean install test
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

${BUILDNINJA}: Makefile CMakeLists.txt
	mkdir -p ${BUILDDIR}
	cd ${BUILDDIR} && cmake ${CMAKEFLAGS} ..

config: ${BUILDNINJA}

build: ${BUILDNINJA}
	ninja -C ${BUILDDIR}

clean:
	ninja -C ${BUILDDIR} clean

distclean:
	rm -rf build

install: build
	DESTDIR=$(abspath ${DESTDIR}) ninja -C ${BUILDDIR} install

test: build
	ninja -C ${BUILDDIR} test

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

