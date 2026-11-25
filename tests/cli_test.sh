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
repeat)
	# the same plot, rendered many times to a file and through a pipe, is
	# the same every time and complete (#40).  In the C locale (no LANG,
	# as in a docker container) printf("%ls") gave up at the legend's
	# non-ASCII symbol, and the plot was cut off mid-line
	n=${CLI_REPEAT:-200}
	cols=80 lines=24
	seq 1 20 | awk '{ print $1, $1*$1 }' > "$tmp/xy"
	# the figure leaves two rows for the prompt, and blot ends the
	# output with a newline after the last frame
	frame=$((lines - 2))
	# one frame clears the screen ("ESC [2J") first: count the lines of
	# the last one, up to the final empty line
	last_frame_lines() {
		awk 'BEGIN { RS = "\033\\[2J" } { f = $0 } END { printf "%s", f }' "$1" \
			| awk 'END { print NR }'
	}
	ref=
	for loc in C C.UTF-8 ""; do
		for how in file pipe; do
			echo "running $n times: blot --ascii line --read xy -p 1,2 ($how, LC_ALL='$loc')"
			i=0
			while [ $i -lt $n ]; do
				i=$((i + 1))
				if [ $how = file ]; then
					LC_ALL=$loc COLUMNS=$cols LINES=$lines "$blot" --ascii line \
						--read "$tmp/xy" -p 1,2 </dev/null > "$tmp/out" 2>"$tmp/err"
					rc=$?
				else
					{ LC_ALL=$loc COLUMNS=$cols LINES=$lines "$blot" --ascii line \
						--read "$tmp/xy" -p 1,2 </dev/null 2>"$tmp/err"; \
					  echo $? > "$tmp/rc"; } | cat > "$tmp/out"
					rc=$(cat "$tmp/rc")
				fi
				cat "$tmp/err" >> "$tmp/out"
				no_sanity
				test "$rc" -eq 0 || fail "run $i: exit code $rc"
				test -s "$tmp/err" && fail "run $i: stderr is not empty"
				if [ -z "$ref" ]; then
					ref=$tmp/ref
					cp "$tmp/out" "$ref"
					got=$(last_frame_lines "$ref")
					test "$got" -eq $((frame + 1)) \
						|| fail "the last frame has $((got - 1)) lines, expected $frame ($(wc -c < "$ref") bytes)"
					# the legend, the last line of the frame, is whole
					tail -n 2 "$ref" | grep -q -F "$tmp/xy" || fail "no legend line"
				fi
				cmp -s "$ref" "$tmp/out" \
					|| fail "run $i ($how, LC_ALL='$loc'): $(wc -c < "$tmp/out") bytes differ from the first run's $(wc -c < "$ref")"
			done
		done
	done
	;;
*)
	echo "unknown case '$case'"; exit 2 ;;
esac
echo PASS
