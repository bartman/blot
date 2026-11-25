#!/bin/sh
# googletest counts per test program, from the log of the last ctest run
# in a build directory (#2): tests run, passed, skipped (GTEST_SKIP, e.g.
# a check for another build) and disabled (DISABLED_*, the known failures
# that name a bug issue), and the totals.
set -e
build=${1:-build}
log=$build/Testing/Temporary/LastTest.log
test -f "$log"
awk '
/^[0-9]+\/[0-9]+ Testing: / { n++; name[n] = $NF; next }
/^\[==========\] [0-9]+ tests? from .* ran/ { run[n] = $2 }
/^\[  PASSED  \] [0-9]+ test/ { ok[n] = $4 }
/^\[  SKIPPED \] [0-9]+ test/ { sk[n] = $4 }
/^  YOU HAVE [0-9]+ DISABLED TESTS?/ { dis[n] = $3 }
END {
	printf "%-24s %5s %7s %8s %9s\n", "program", "run", "passed", "skipped", "disabled"
	for (i = 1; i <= n; i++) {
		printf "%-24s %5d %7d %8d %9d\n", name[i], run[i], ok[i], sk[i], dis[i]
		R += run[i]; P += ok[i]; S += sk[i]; D += dis[i]
	}
	printf "%-24s %5d %7d %8d %9d\n", "TOTAL", R, P, S, D
}' "$log"
