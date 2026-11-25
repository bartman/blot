#!/bin/sh
# Line and branch coverage of lib/*.c, from a gcc --coverage build whose
# tests have run (#2).  Plain gcov, no gcovr/lcov: they aren't in the CI
# images.  The last line is what the coverage job's coverage: regex reads:
#   TOTAL lines: NN.N% (covered/total), branches: NN.N% (taken/total)
#
#   cmake -Bbuild-cov -S. -G Ninja -DCMAKE_BUILD_TYPE=Debug \
#         -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DENABLE_IPO=OFF \
#         -DCMAKE_C_FLAGS="--coverage -O0" -DCMAKE_CXX_FLAGS="--coverage -O0"
#   ninja -C build-cov && (cd build-cov && ctest)
#   sh ci/coverage.sh build-cov
#
# Only the library's own objects are read, so this is lib/*.c; the static
# inline code of include/*.h is also compiled into the test programs and
# isn't counted here.
set -e
build=${1:-build-cov}
objs=$build/lib/CMakeFiles/blot_so.dir
ls "$objs"/*.gcda >/dev/null

# gcov -n: print the summary, write no .gcov files
gcov -n -b -o "$objs" "$objs"/*.gcda | awk '
function frac(s) {              # "Lines executed:96.00% of 200" -> covered
	sub(/^[^:]*:/, "", s); split(s, a, /% of /)
	return int(a[1] * a[2] / 100 + 0.5)
}
function total(s) { sub(/.*% of /, "", s); return s + 0 }
function pct(c, t) { return t ? sprintf("%.1f", 100 * c / t) : "-" }
/^File / {
	f = $0; sub(/^File ./, "", f); sub(/.$/, "", f)
	keep = (f ~ /\/lib\/[^\/]*\.c$/ && !(f in seen))
	if (keep) { seen[f] = 1; name = f; sub(/.*\/lib\//, "lib/", name); n++; names[n] = name }
	next
}
# one of each per file; the last "Lines executed" is the gcov total
keep && /^Lines executed:/ && !(name in lt)      { lc[name] = frac($0); lt[name] = total($0) }
keep && /^Taken at least once:/ && !(name in bt) { bc[name] = frac($0); bt[name] = total($0) }
END {
	for (i = 1; i <= n; i++) {
		f = names[i]
		printf "%-22s lines %4d/%-4d %5s%%   branches %4d/%-4d %5s%%\n", f, lc[f], lt[f], pct(lc[f], lt[f]), bc[f], bt[f], pct(bc[f], bt[f])
		LC += lc[f]; LT += lt[f]; BC += bc[f]; BT += bt[f]
	}
	if (!LT) { print "no coverage data for lib/*.c"; exit 1 }
	printf "TOTAL lines: %s%% (%d/%d), branches: %s%% (%d/%d)\n", pct(LC, LT), LC, LT, pct(BC, BT), BC, BT
}'
