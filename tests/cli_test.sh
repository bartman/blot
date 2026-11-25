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

unset COLUMNS LINES NO_COLOR
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
	echo "running: blot $* </dev/null | cat  (COLUMNS=${COLUMNS-unset} LINES=${LINES-unset})"
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

# $1: the expected exit code; the rest: blot's arguments.  Like expect,
# but with a terminal size (COLUMNS and LINES), so it plots, and stdout
# alone in $tmp/out (stderr in $tmp/err): the plot, through a pipe
plot() {
	want=$1; shift
	echo "running: blot $* </dev/null | cat  (COLUMNS=40 LINES=12)"
	{ COLUMNS=40 LINES=12 "$blot" "$@" </dev/null 2>"$tmp/err"; echo $? > "$tmp/rc"; } \
		| cat > "$tmp/out"
	rc=$(cat "$tmp/rc")
	echo "exit code: $rc"
	cat "$tmp/err" >> "$tmp/out"
	no_sanity
	test "$rc" -eq "$want" || fail "exit code $rc, expected $want"
	if [ "$want" -eq 0 ]; then
		test -s "$tmp/err" && fail "stderr is not empty"
		grep -q -e '+[*-]*----' "$tmp/out" || fail "no X axis: no plot"
	fi
}

ESC=$(printf '\033')
# no escape sequence at all in $tmp/out: no colour, no screen clear (#44)
no_escapes() {
	if grep -q "$ESC" "$tmp/out"; then
		fail "an escape sequence in the output ($*)"
	fi
}
# colour (SGR) escapes in $tmp/out (#44)
has_colour() {
	grep -q "$ESC\[[0-9;]*m" "$tmp/out" || fail "no colour in the output ($*)"
}
# the number of frames in $tmp/out: each starts with a screen clear
frames() {
	grep -o "$ESC\[2J" "$tmp/out" | wc -l
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
	# complete and the same every time, in any locale (#40).  In the C
	# locale (no LANG, as in a docker container) printf("%ls") gave up at
	# the first non-ASCII character and the plot was cut off mid-line.
	# --ascii has one (the legend's symbol, #46); the default Unicode
	# mode is non-ASCII throughout, so it keeps the test meaningful
	n=${CLI_REPEAT:-100}
	cols=80 lines=24
	seq 1 20 | awk '{ print $1, $1*$1 }' > "$tmp/xy"
	# the figure leaves two rows for the prompt, and blot ends the
	# output with a newline after the last frame
	frame=$((lines - 2))
	# blot draws a frame every second as well as at the end (the -i
	# interval can't be changed, #44), so the number of frames depends on
	# the clock: compare only the last frame, which follows the last
	# "ESC [2J" (clear screen)
	last_frame() {
		awk 'BEGIN { RS = "\033\\[2J" } { f = $0 } END { printf "%s", f }' "$1"
	}
	# the inherited locale: C or POSIX (blot uses UTF-8 there) or a
	# UTF-8 one; another one writes some characters as '?' and warns
	locs="C C.UTF-8"
	case $(locale charmap 2>/dev/null) in
	UTF-8|ANSI_X3.4-1968) locs="$locs inherited" ;;
	*) echo "skipping the inherited locale: its charmap is '$(locale charmap 2>/dev/null)', not UTF-8" ;;
	esac
	for mode in --ascii --unicode; do
		ref=
		for loc in $locs; do
			test "$loc" = inherited && loc= || true
			for how in file pipe; do
				echo "running $n times: blot $mode line --read xy -p 1,2 ($how, LC_ALL='$loc')"
				i=0
				while [ $i -lt $n ]; do
					i=$((i + 1))
					if [ $how = file ]; then
						LC_ALL=$loc COLUMNS=$cols LINES=$lines "$blot" $mode line \
							--read "$tmp/xy" -p 1,2 </dev/null > "$tmp/out" 2>"$tmp/err"
						rc=$?
					else
						{ LC_ALL=$loc COLUMNS=$cols LINES=$lines "$blot" $mode line \
							--read "$tmp/xy" -p 1,2 </dev/null 2>"$tmp/err"; \
						  echo $? > "$tmp/rc"; } | cat > "$tmp/out"
						rc=$(cat "$tmp/rc")
					fi
					cat "$tmp/err" >> "$tmp/out"
					no_sanity
					test "$rc" -eq 0 || fail "run $i: exit code $rc"
					test -s "$tmp/err" && fail "run $i: stderr is not empty"
					last_frame "$tmp/out" > "$tmp/frame"
					if [ -z "$ref" ]; then
						ref=$tmp/ref
						cp "$tmp/frame" "$ref"
						got=$(awk 'END { print NR }' "$ref")
						test "$got" -eq $((frame + 1)) \
							|| fail "$mode: the last frame has $((got - 1)) lines, expected $frame ($(wc -c < "$tmp/out") bytes)"
						# the legend, the last line of the frame, is whole
						tail -n 2 "$ref" | grep -q -F "$tmp/xy" \
							|| fail "$mode: no legend line"
						# the Unicode plot is written in UTF-8, not ASCII
						if [ $mode = --unicode ]; then
							LC_ALL=C grep -q '[^ -~[:cntrl:]]' "$ref" \
								|| fail "$mode: no non-ASCII characters in the plot"
							LC_ALL=C grep -q '?' "$ref" \
								&& fail "$mode: characters written as '?'"
						fi
					fi
					cmp -s "$ref" "$tmp/frame" \
						|| fail "run $i ($mode, $how, LC_ALL='$loc'): the last frame ($(wc -c < "$tmp/frame") bytes) differs from the first run's ($(wc -c < "$ref") bytes)"
				done
			done
		done
	done
	;;
write_error)
	# a write error is a message on stderr and exit code 1, not a
	# silently lost plot (#40)
	if [ ! -c /dev/full ]; then
		echo "SKIP: no /dev/full"; exit 0
	fi
	printf '1 1\n2 4\n' > "$tmp/xy"
	echo "running: blot --ascii line --read xy >/dev/full"
	COLUMNS=80 LINES=24 "$blot" --ascii line --read "$tmp/xy" </dev/null \
		>/dev/full 2>"$tmp/out"
	rc=$?
	echo "exit code: $rc"
	no_sanity
	test "$rc" -eq 1 || fail "exit code $rc, expected 1"
	grep -q 'cannot write the plot' "$tmp/out" || fail "no message about the write error"
	;;
color_never)
	# --color=never and --no-color: no escape sequence at all (#44)
	plot 0 --color=never line --read "$tmp/data"
	no_escapes "--color=never"
	plot 0 --no-color line --read "$tmp/data"
	no_escapes "--no-color"
	# the last one given wins
	plot 0 --color=always --no-color line --read "$tmp/data"
	no_escapes "--color=always --no-color"
	;;
no_color_env)
	# NO_COLOR (https://no-color.org): set and not empty, no colour (#44);
	# --color=always still has it
	export NO_COLOR=1
	plot 0 line --read "$tmp/data"
	no_escapes "NO_COLOR=1"
	plot 0 --color=auto line --read "$tmp/data"
	no_escapes "NO_COLOR=1 --color=auto"
	plot 0 --color=always line --read "$tmp/data"
	has_colour "NO_COLOR=1 --color=always"
	;;
color_auto)
	# the default, and --color=auto: colour only on a terminal; stdout is
	# a pipe here, so none, and no screen clear either (#44)
	plot 0 line --read "$tmp/data"
	no_escapes "the default, piped"
	plot 0 --color=auto line --read "$tmp/data"
	no_escapes "--color=auto, piped"
	;;
color_always)
	# --color=always: colour and the screen clear, even through a pipe (#44)
	plot 0 --color=always line --read "$tmp/data"
	has_colour "--color=always"
	test "$(frames)" -ge 1 || fail "no screen clear with --color=always"
	;;
color_bad)
	export COLUMNS=40 LINES=12
	expect 1 --color=sometimes line --read "$tmp/data"
	grep -q -F 'sometimes' "$tmp/out" || fail "no message about the --color value"
	;;
interval)
	# --interval 0.5: a frame every 0.5 s while the input lasts, then the
	# last one (#44).  The command takes 3.2 s: 6 frames at 0.5 s (7 with
	# the last); at the default 1 s (or with the value ignored) 3, or 4.
	# A slow machine draws fewer, not more, so the bound is the low one
	cmd='echo 1; sleep 3.2; echo 2'
	plot 0 --color=always --interval 0.5 line --exec "$cmd"
	n=$(frames)
	echo "--interval 0.5: $n frames"
	test "$n" -ge 6 || fail "$n frames in 3.2 s at --interval 0.5, expected at least 6"
	plot 0 --color=always line --exec "$cmd"
	n=$(frames)
	echo "the default interval: $n frames"
	test "$n" -le 4 || fail "$n frames in 3.2 s at the default 1 s, expected at most 4"
	;;
interval_bad)
	# not a positive finite number of seconds: an error, exit code 1 (#44)
	export COLUMNS=40 LINES=12
	for bad in 0 -1 abc 0.5s inf nan 1e999; do
		expect 1 --interval "$bad" line --read "$tmp/data"
		grep -q -i 'interval' "$tmp/out" || fail "no message about --interval '$bad'"
	done
	;;
one_frame)
	# a file read to the end, in less than the interval, is one frame
	# with all of it: blot drew a first frame with only the first value,
	# which looked like a one-column file plotted only its first (#44,
	# #47).  --color=always to have the screen clears that start them
	printf '1\n3\n2\n5\n' > "$tmp/column"
	plot 0 --color=always line --read "$tmp/column"
	n=$(frames)
	test "$n" -eq 1 || fail "$n frames for a file read to the end, expected 1"
	;;
*)
	echo "unknown case '$case'"; exit 2 ;;
esac
echo PASS
