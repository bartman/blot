#!/bin/sh
# googletest counts per test program, from the log of the last ctest run
# in a build directory (#2): tests run, passed and disabled (DISABLED_*,
# the known failures that name a bug issue), and the totals.
set -e
build=${1:-build}
log=$build/Testing/Temporary/LastTest.log
test -f "$log"
awk '
/^[0-9]+\/[0-9]+ Testing: / { n++; name[n] = $NF; next }
/^\[==========\] [0-9]+ tests? from .* ran/ { run[n] = $2 }
/^\[  PASSED  \] [0-9]+ test/ { ok[n] = $4 }
/^  YOU HAVE [0-9]+ DISABLED TESTS?/ { dis[n] = $3 }
END {
	printf "%-24s %5s %7s %9s\n", "program", "run", "passed", "disabled"
	for (i = 1; i <= n; i++) {
		printf "%-24s %5d %7d %9d\n", name[i], run[i], ok[i], dis[i]
		R += run[i]; P += ok[i]; D += dis[i]
	}
	printf "%-24s %5d %7d %9d\n", "TOTAL", R, P, D
}' "$log"

# The programs that ctest failed although no gtest in them failed (#30),
# which the counts above can't show: one that ran every test and passed
# them all but then failed (a leak at exit under LeakSanitizer, a crash
# in a static destructor, a nonzero exit status), or one that stopped
# before gtest's summary (a crash, abort or timeout in a test), so its
# counts are those of the tests before.  A program with a gtest failure
# isn't listed: its counts already show it.
awk '
/^[0-9]+\/[0-9]+ Testing: / { n++; name[n] = $NF; out = 1; next }
/^<end of output>$/ { out = 0; next }
out && /^\[==========\] [0-9]+ tests? from .* ran/ { ran[n] = 1 }
out && /^\[  FAILED  \] [0-9]+ tests?, listed below/ { gfail[n] = 1 }
!out && /^Test Failed\.$/ { cfail[n] = 1 }
END {
	k = 0
	for (i = 1; i <= n; i++) {
		if (!cfail[i] || gfail[i])
			continue
		if (!k++)
			print "failed under ctest with no gtest failure:"
		if (ran[i])
			printf "  %-22s every gtest passed, then the program failed (exit status, leak at exit, ...)\n", name[i]
		else
			printf "  %-22s stopped before gtest'"'"'s summary (crash, abort or timeout)\n", name[i]
	}
	if (!k)
		print "failed under ctest with no gtest failure: none"
}' "$log"
