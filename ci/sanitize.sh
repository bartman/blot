#!/bin/sh
# sanitize.sh [prefix]: build blot with clang's sanitizers and run the
# unit tests (#15), for the sanitize-deb13 job of .gitlab-ci.yml.
#
# Two builds, because one can hide what the other finds:
#   <prefix>-asan    -fsanitize=address,undefined
#   <prefix>-ubsan   -fsanitize=undefined
# UBSan's alignment check depends on the allocator: ASan's allocator puts
# larger blocks at 64-byte boundaries by chance, so a struct that needs
# 64-byte alignment but comes from plain malloc (16 bytes) shows only in
# the build without ASan, on glibc's malloc.
#
# -fno-sanitize-recover=undefined makes the first UB a test failure (the
# program aborts); ASan always stops at its first error.  Only the enabled
# tests run, as in unit-test-*; the DISABLED_ ones are known failures.
#
# LeakSanitizer is on in the ASan build (detect_leaks=1) and checks at
# each test program's exit.  RUN_ISOLATED (tests/blot_test.hpp) runs a
# body in a death-test child that ends with _exit(), which skips that
# check, so the child runs it itself before _exit()
# (__lsan_do_recoverable_leak_check).  A sanitizer report or a leak in
# the child fails that test (the child doesn't exit 0), and gtest shows
# the child's stderr.
#
# Both builds always run; the script fails when either one does, after a
# summary of the sanitizer reports of each.  The sanitize-deb13 job gates
# on that exit status (#19).
set -u
prefix=${1:-build-san}
src=$(cd "$(dirname "$0")/.." && pwd)

# a report names the file and line; print_stacktrace shows how it got there
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-print_stacktrace=1}
export ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=1}

clang --version | head -1

rc=0
for kind in asan ubsan; do
	case $kind in
	asan)  san=address,undefined ;;
	ubsan) san=undefined ;;
	esac
	dir=$prefix-$kind
	flags="-fsanitize=$san -fno-sanitize-recover=undefined -fno-omit-frame-pointer -O1 -g"
	echo "### $dir: -fsanitize=$san"
	# ENABLE_IPO=OFF: clang's LTO would need the gold plugin or lld
	if ! cmake -B"$dir" -S"$src" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
			-DENABLE_IPO=OFF \
			"-DCMAKE_C_FLAGS=$flags" "-DCMAKE_CXX_FLAGS=$flags" \
			"-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=$san" \
			"-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=$san" \
		|| ! ninja -C "$dir"; then
		echo "### $dir: build failed"
		rc=1
		continue
	fi
	(cd "$dir" && ctest --output-on-failure --output-junit test_report.xml) || rc=1
done

echo
echo "### summary"
for kind in asan ubsan; do
	dir=$prefix-$kind
	log=$dir/Testing/Temporary/LastTest.log
	echo "## $dir"
	if [ ! -f "$log" ]; then
		echo "no test log"
		continue
	fi
	sh "$src/ci/test-summary.sh" "$dir"
	# one line per distinct report, without the build's path or addresses
	reports=$(grep -E 'runtime error:|ERROR: [A-Za-z]+Sanitizer' "$log" \
		| sed -e "s#$src/##g" -e 's/^.*\(lib\/\|include\/\|tests\/\|cli\/\)/\1/' \
			-e 's/0x[0-9a-f]*/0x.../g' -e 's/==[0-9]*==//' \
		| sort | uniq -c)
	if [ -n "$reports" ]; then
		echo "sanitizer reports (count, report):"
		echo "$reports"
	else
		echo "sanitizer reports: none"
	fi
done
exit $rc
