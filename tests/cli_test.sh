#!/bin/sh
# Runs the blot CLI with stdout a pipe, stdin /dev/null and COLUMNS and
# LINES unset, so blot can't get a terminal size (#37).  It must not end
# in std::terminate (exit 134): --help prints the help and exits 0, and
# a failure prints a message and exits 1.
#
#   cli_test.sh <path to blot> <case>
#
# The cases are added to ctest in tests/CMakeLists.txt, one test each.
set -u
blot=$1
case=$2

unset COLUMNS LINES
# a sanitizer build (ci/sanitize.sh) exits 1 on a report, the code the
# failure cases expect: make it 99, and fail on any report (no_sanity)
export ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}exitcode=99"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}exitcode=99"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
printf '1\n2\n3\n' > "$tmp/data"

fail() { echo "FAIL: $*"; echo "--- output:"; cat "$tmp/out"; exit 1; }

# no sanitizer report in $tmp/out
no_sanity() {
	if grep -q -E 'Sanitizer|runtime error:' "$tmp/out"; then
		fail "sanitizer report"
	fi
}

# $1: the expected exit code; the rest: blot's arguments
expect() {
	want=$1; shift
	echo "running: blot $* </dev/null | cat  (COLUMNS, LINES unset)"
	# stdout and stderr through a pipe, not a tty; blot's exit code,
	# not cat's, through a file
	{ "$blot" "$@" </dev/null 2>&1; echo $? > "$tmp/rc"; } | cat > "$tmp/out"
	rc=$(cat "$tmp/rc")
	echo "exit code: $rc"
	no_sanity
	test "$rc" -eq "$want" || fail "exit code $rc, expected $want"
	if grep -q -e 'terminate called' -e 'Aborted' "$tmp/out"; then
		fail "std::terminate"
	fi
}

case $case in
help)
	expect 0 --help
	grep -q '^SYNOPSIS' "$tmp/out" || fail "no SYNOPSIS in the help"
	grep -q -- '--help' "$tmp/out" || fail "no --help in the help"
	grep -q '^EXAMPLES' "$tmp/out" || fail "no EXAMPLES in the help"
	# the help is wrapped at 80 columns when there is no terminal
	long=$(awk 'length($0) > 80' "$tmp/out" | head -1)
	test -z "$long" || fail "a help line is longer than 80 columns: '$long'"
	;;
render)
	# a plot needs a terminal size, from the tty or COLUMNS and LINES
	expect 1 line --read "$tmp/data"
	grep -q 'terminal size' "$tmp/out" || fail "no message about the terminal size"
	# with COLUMNS and LINES set it plots
	COLUMNS=60 LINES=20 "$blot" --ascii line --read "$tmp/data" </dev/null > "$tmp/out" 2>&1 \
		|| fail "exit code $? with COLUMNS=60 LINES=20"
	no_sanity
	;;
regex)
	# std::regex_error from a bad --regex, a std::exception
	expect 1 line --read "$tmp/data" --regex '('
	grep -q -i -E 'regex|regular expression|parenthes' "$tmp/out" || fail "no message about the regex"
	;;
*)
	echo "unknown case '$case'"; exit 2 ;;
esac
echo PASS
