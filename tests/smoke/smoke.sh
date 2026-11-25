#!/bin/sh
# Smoke tests (#38): run the blot CLI and the example programs end to end,
# the way a user does, with stdout a pipe and the terminal size from
# COLUMNS and LINES.
#
#   smoke.sh [build dir]          (make smoke runs it on build/)
#
# BLOT=<path> tests another blot (e.g. an installed /usr/bin/blot) and
# EXAMPLES=<dir> other examples (with c/ and cpp/ in it).  On a
# sanitizer build its reports fail the run; today that build fails on
# the UB in the *-trig-loop examples (#43) until that is fixed.  CI's
# smoke jobs run the plain build.
#
# The CLI
#   - plots the inputs in this directory as line, scatter and bar, read
#     from a file (--read) and from stdin (--read /dev/stdin), and the
#     last frame it prints must match <case>.out here, byte for byte once
#     the colour escapes are taken out (the CLI has no flag to turn
#     colour off).  Every .out was checked by hand against its data: see
#     the comments at each case below.  After a change that moves the
#     output on purpose, SMOKE_UPDATE=1 writes the .out files again;
#     check the new ones by hand, the same way, before committing them;
#   - --help and --version, with stdout a pipe.
# The examples (examples/c and examples/cpp) each print a screen whose
# size the table below gives, from the code: the number of lines and the
# width of the widest line must be exactly that, with no nan or inf, and
# the exit code 0.  The *-loop and *-line-spin examples draw until
# SIGINT: they get one after a few seconds and must then exit 0.
#
# Any mismatch, a non-zero exit code or a sanitizer report fails the run
# (exit 1), after all the checks have run.
set -u

here=$(cd "$(dirname "$0")" && pwd)
build=${1:-build}
blot=${BLOT:-$build/cli/blot}
examples=${EXAMPLES:-$build/examples}
# the CLI runs in this directory (the legend shows the file names)
case $blot in /*) ;; */*) blot=$PWD/$blot ;; *) blot=$(command -v "$blot") ;; esac

# fixed sizes and locale: the output must not depend on where it runs.
# The CLI goldens also run a second time with LC_ALL=C (no locale, as in
# docker and CI) and must match the same .out files byte for byte: blot
# writes UTF-8 there too, and no longer loses the plot at the first
# non-ASCII character (#40)
COLUMNS=40
LINES=16
LC_ALL=C.UTF-8
export COLUMNS LINES LC_ALL
unset LANG LANGUAGE NO_COLOR
# a sanitizer build: never exit 0 on a report (exit codes are checked)
export ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}exitcode=99"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}exitcode=99:print_stacktrace=1"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

failed=0
passed=0
fail() {
	echo "FAIL: $name: $*"
	failed=$((failed+1))
}
pass() {
	echo "PASS: $name"
	passed=$((passed+1))
}

# $tmp/out without escape sequences, into $tmp/text: SGR (colour) only;
# any other escape but the clear (ESC[2J ESC[H) fails
strip() {
	ESC=$(printf '\033')
	sed -e "s/$ESC\[[0-9;]*m//g" -e "s/$ESC\[2J$ESC\[H//g" "$tmp/out" > "$tmp/text"
	if grep -q "$ESC" "$tmp/text"; then
		fail "an unexpected escape sequence: $(grep -m1 "$ESC" "$tmp/text" | od -c | head -2)"
		return 1
	fi
}

# common checks of $tmp/out and exit code $rc against $1 (the expected
# exit code)
check_run() {
	ok=0
	if grep -q -a -E 'Sanitizer|runtime error:' "$tmp/out"; then
		fail "sanitizer report:"; sed -n '1,40p' "$tmp/out"; ok=1
	fi
	if grep -q -a -e 'terminate called' "$tmp/out"; then
		fail "std::terminate"; ok=1
	fi
	if [ "$rc" -ne "$1" ]; then
		fail "exit code $rc, expected $1"; sed -n '1,20p' "$tmp/out"; ok=1
	fi
	return $ok
}

# ---------------------------------------------------------------------------
# the CLI

# $1: the case, the rest: blot's arguments; stdin is the caller's.
# Compares the last frame, without colour, to $here/$1.out.
golden() {
	case=$1; shift
	name=cli-$case${golden_suffix:-}
	echo "--- $name: blot $*"
	(cd "$here" && timeout -k 5 30 "$blot" "$@") > "$tmp/out" 2>&1
	rc=$?
	check_run 0 || return
	strip || return
	# the last frame: the CLI clears the screen for each, and prints one
	# when it starts reading and one when the input ends (a slow run may
	# print more in between)
	ESC=$(printf '\033')
	sed "s/$ESC\[2J$ESC\[H/@@FRAME@@\n/g" "$tmp/out" \
		| awk '/^@@FRAME@@$/ { n = 0; delete f; next } { f[n++] = $0 } END { for (i = 0; i < n; i++) print f[i] }' \
		| sed "s/$ESC\[[0-9;]*m//g" > "$tmp/last"
	if [ -n "${SMOKE_UPDATE:-}" ] && [ -z "${golden_suffix:-}" ]; then
		cp "$tmp/last" "$here/$case.out"
		echo "updated $here/$case.out"
	fi
	if ! cmp -s "$here/$case.out" "$tmp/last"; then
		diff -u "$here/$case.out" "$tmp/last" > "$tmp/diff"
		fail "the last frame differs from $case.out (- expected, + got):"
		cat "$tmp/diff"
		return
	fi
	# the screen: LINES-2 rows of COLUMNS-2 cells, then the empty line the
	# CLI prints at the end
	lines=$(wc -l < "$tmp/last")
	width=$(wc -L < "$tmp/last")
	if [ "$lines" -ne $((LINES-1)) ] || [ "$width" -ne $((COLUMNS-2)) ]; then
		fail "$lines lines of $width columns, expected $((LINES-1)) of $((COLUMNS-2))"
		return
	fi
	cat "$tmp/last"
	pass
}

if [ ! -x "$blot" ]; then
	echo "FAIL: no blot at $blot"
	exit 1
fi

# The .out files were checked by hand against the axis mapping (and the
# cells recomputed from it with a script, both ways: every value in its
# cell, nothing marked anywhere else).  An axis goes from min - 0.1% of
# the range to max + 0.1%; a value v is in cell
# floor((v - min) * len / (max - min)), the last cell for max, counted
# from the left and from the bottom.  The screen is 38x14; the plot area
# is what the Y labels (and the axis column), the X axis, its labels and
# the legend leave: 29 columns (30 with 7-character labels) by 11 rows
# (10 with two legend lines).  The ticks split the axis in 3; a label is
# the value at its tick, with 0 on a tick when the range spans it.

cli_goldens() {
# line, from a file: 11 values 0..10 at x = 1..11 (the line numbers);
# x 0.99..11.01, y -0.01..10.01, 29x11, so (1 + k, k) is in column
# floor((k + 0.01) * 29 / 10.02) and row floor((k + 0.01) * 11 / 10.02):
# (0,0) (2,1) (5,2) (8,3) (11,4) (14,5) (17,6) (20,7) (23,8) (26,9)
# (28,10), and the line joins them through the cells between; the X
# labels are 0.990, 4.330, 7.670, the Y labels 0.000, 3.340, 6.680
golden line-file --ascii line --read ramp.txt < /dev/null

# scatter, x,y pairs from stdin: the corners and the centre of
# 0..10 x 0..10, 29x11: (0,0) in column 0 row 0, (10,0) in column 28
# (the last) row 0, (0,10) column 0 row 10, (10,10) column 28 row 10,
# and (5,5) in column floor(5.01 * 29 / 10.02) = 14 and row
# floor(5.01 * 11 / 10.02) = 5; nothing else
golden scatter-stdin --ascii scatter --read /dev/stdin --position 1,2 < "$here/corners.txt"

# bar, from a file: 1 3 2 4 at x = 1..4; y 0.997..4.003 on 11 rows, so
# the tops are in rows 0, 7, 3 and 10, each bar filled down to row 0.
# X is not the axis mapping but the bar slot model (blot_layer_bar, the
# open question of #12): 4.006 slots of 30/4.006 columns, a bar in the
# left half of its slot: columns 0..4, 8..11, 15..19, 22..26, so the
# bars sit left of the X ticks (0.997, 1.999, 3.001 at columns 0, 10, 20).
# This golden bakes in the current slot model, which the owner is
# deciding in #12: a change there regenerates it (SMOKE_UPDATE=1) and
# re-checks it by hand
golden bar-file --ascii bar --read bars.txt < /dev/null

# line, from a one-column file (#47): 1 3 2 5 at x = 1..4, all four
# values, not only the first; x 0.997..4.003, y 0.996..5.004, 30x11 (the
# Y labels are 6 characters), so (1,1) (2,3) (3,2) (4,5) are in columns
# floor((x - 0.997) * 30 / 3.006) = 0, 10, 19, 29 and rows
# floor((y - 0.996) * 11 / 4.008) = 0, 5, 2, 10, and the line joins
# them; the X labels 0.997, 1.999, 3.001, the Y labels 0.996, 2.332, 3.668
golden line-column --ascii line --read column.txt < /dev/null

# two layers on one figure, the legend lists both: the ramp's line and
# the corners' scatter; x -0.011..11.011 (0..11 over both), y
# -0.01..10.01, 29x10: the corners in columns 0, 26, 13 (centre) and rows
# 0, 9, 5; the ramp's points in columns 2 5 7 10 13 15 18 21 23 26 28 and
# rows 0 1 2 3 4 5 5 6 7 8 9 (6 is floor(6.01 * 10 / 10.02) = 5); the X
# labels 0.000, 3.674, 7.348
golden two-layers --ascii line --read ramp.txt scatter --read /dev/stdin -p 1,2 < "$here/corners.txt"
}

# in the fixed UTF-8 locale, then in the C locale against the same .out
# files (#40)
golden_suffix=
cli_goldens
golden_suffix=-C
LC_ALL=C
cli_goldens
golden_suffix=
LC_ALL=C.UTF-8

# --help and --version through a pipe (#37): exit 0, and the text
name=cli-help
echo "--- $name: blot --help | cat"
{ timeout -k 5 30 "$blot" --help < /dev/null 2>&1; echo $? > "$tmp/rc"; } | cat > "$tmp/out"
rc=$(cat "$tmp/rc")
if check_run 0; then
	if ! grep -q '^SYNOPSIS' "$tmp/out" || ! grep -q '^EXAMPLES' "$tmp/out" \
	   || ! grep -q -- '--read' "$tmp/out"; then
		fail "no SYNOPSIS, EXAMPLES or --read in the help"; cat "$tmp/out"
	else
		pass
	fi
fi

name=cli-version
echo "--- $name: blot --version | cat"
{ timeout -k 5 30 "$blot" --version < /dev/null 2>&1; echo $? > "$tmp/rc"; } | cat > "$tmp/out"
rc=$(cat "$tmp/rc")
if check_run 0; then
	cat "$tmp/out"
	if ! grep -q '^refspec: ' "$tmp/out" || ! grep -q '^revision: ' "$tmp/out" \
	   || ! grep -q -E '^sha1: [0-9a-f]{40}$' "$tmp/out"; then
		fail "no refspec, revision or sha1 line"
	else
		pass
	fi
fi

# ---------------------------------------------------------------------------
# the examples

# name, how it runs (once, or loop: until SIGINT), lines (per frame for a
# loop), width; from the code.  The library's screen is the terminal less
# 2 rows and 2 columns; the others set their screen size:
#   histogram  80x40
#   simple     80x40, between two lines of 80 dashes
#   trig       80x40, then an empty line (printf "%ls\n")
#   x          the library's: (COLUMNS-2)x(LINES-2)
#   trig-loop  80x20, an empty line, two timing lines; per frame
#   line-spin  COLUMNS x (LINES-11), two timing lines; per frame (the
#              C++ one ends with an empty line and DONE)
examples_table() {
	cat <<-EOF
	histogram  once 40 80
	simple     once 42 80
	trig       once 41 80
	x          once $((LINES-2)) $((COLUMNS-2))
	trig-loop  loop 23 80
	line-spin  loop $((LINES-11+2)) $COLUMNS
	EOF
}

# the examples run larger than the CLI: they need room (line-spin wants
# more than 11 rows), and a size other than 80 columns shows which of
# them follow the terminal
COLUMNS=100
LINES=30

examples_table > "$tmp/table"
while read -r base how want_lines want_width; do
	for lang in c cpp; do
		name=$lang-$base
		prog=$examples/$lang/$name
		echo "--- $name: COLUMNS=$COLUMNS LINES=$LINES $prog ($how)"
		if [ ! -x "$prog" ]; then
			fail "no $prog"; continue
		fi
		if [ "$how" = loop ]; then
			# a few seconds, then SIGINT: it finishes the frame and
			# exits 0; KILL if it doesn't
			timeout --preserve-status -k 10 -s INT 3 "$prog" < /dev/null > "$tmp/out" 2>&1
		else
			timeout -k 10 60 "$prog" < /dev/null > "$tmp/out" 2>&1
		fi
		rc=$?
		check_run 0 || continue
		strip || continue
		if grep -q -i -w -E 'nan|inf' "$tmp/text"; then
			fail "nan or inf in the output:"; grep -i -w -E 'nan|inf' "$tmp/text" | head -5
			continue
		fi
		width=$(wc -L < "$tmp/text")
		if [ "$how" = loop ]; then
			# lines per frame, each frame starts with a clear
			ESC=$(printf '\033')
			# (cpp-line-spin ends with an empty line and DONE)
			sed "s/$ESC\[2J$ESC\[H/@@FRAME@@\n/g" "$tmp/out" \
				| awk -v want="$want_lines" '
					/^@@FRAME@@$/ { if (frames && n != want) bad++; frames++; n = 0; next }
					{ n++; prev = cur; cur = $0 }
					END { if (cur == "DONE" && prev == "") n -= 2
					      if (frames && n != want) bad++;
					      printf "%d %d %d\n", frames, bad + 0, n }' > "$tmp/frames"
			read -r frames bad last < "$tmp/frames"
			if [ "$frames" -lt 2 ] || [ "$bad" -ne 0 ]; then
				fail "$frames frames, $bad without $want_lines lines (the last has $last)"
				continue
			fi
			if [ "$width" -ne "$want_width" ]; then
				fail "$width columns, expected $want_width"; continue
			fi
			echo "$frames frames of $want_lines lines, $width columns"
		else
			lines=$(wc -l < "$tmp/text")
			if [ "$lines" -ne "$want_lines" ] || [ "$width" -ne "$want_width" ]; then
				fail "$lines lines of $width columns, expected $want_lines of $want_width"
				continue
			fi
			echo "$lines lines, $width columns"
		fi
		pass
	done
done < "$tmp/table"

echo
echo "smoke: $passed passed, $failed failed"
test "$failed" -eq 0
