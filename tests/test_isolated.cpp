/* blot: RUN_ISOLATED shows why its child failed (#26)
 *
 * A body that fails on purpose can't be a normal test, so the failing
 * bodies are in a helper program, isolated_failing (isolated_failing.cpp),
 * and each test here runs one of its cases and checks the output, as a
 * user would see it: the failed assertion's file:line and message, the
 * signal that killed the child, LeakSanitizer's report.
 *
 * Not EXPECT_NONFATAL_FAILURE around RUN_ISOLATED: its fake reporter
 * would also catch the failures in the child, which then exits 0. */
#include "blot_test.hpp"

#include <sys/wait.h>

#include <fstream>

namespace {

struct Helper {
    int status = -1;
    std::string out;    // stdout and stderr
};

Helper run_case(const char *name)
{
    Helper r;
    std::string cmd = std::string("'") + BLOT_ISOLATED_HELPER + "' --gtest_filter=Isolated."
                      + name + " 2>&1";
    FILE *p = popen(cmd.c_str(), "r");
    if (!p) {
        ADD_FAILURE() << "popen: " << cmd;
        return r;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0)
        r.out.append(buf, n);
    r.status = pclose(p);
    return r;
}

/* "isolated_failing.cpp:<line>:", the line in the helper that has the marker */
std::string where(const char *marker)
{
    std::ifstream src(BLOT_ISOLATED_SOURCE);
    std::string line;
    for (int n = 1; std::getline(src, line); n++)
        if (line.find(marker) != std::string::npos)
            return "isolated_failing.cpp:" + std::to_string(n) + ":";
    ADD_FAILURE() << marker << " not found in " << BLOT_ISOLATED_SOURCE;
    return "(no marker)";
}

/* the output, indented: ci/test-summary.sh reads gtest's lines at the
 * start of a line, and these are the helper's */
std::string show(const Helper &r)
{
    std::string s = "helper output:\n";
    size_t start = 0;
    while (start < r.out.size()) {
        size_t nl = r.out.find('\n', start);
        if (nl == std::string::npos)
            nl = r.out.size();
        s += "  | " + r.out.substr(start, nl - start) + "\n";
        start = nl + 1;
    }
    return s;
}

bool has(const Helper &r, const std::string &s)
{
    return r.out.find(s) != std::string::npos;
}

/* the helper ran the case, and it failed */
void expect_failed(const Helper &r, const char *name)
{
    EXPECT_TRUE(WIFEXITED(r.status) && WEXITSTATUS(r.status) == 1) << show(r);
    EXPECT_TRUE(has(r, std::string("[  FAILED  ] Isolated.") + name + " (")) << show(r);
}

} // namespace

TEST(IsolatedOutput, expect_failure_shows_file_line_and_message)
{
    Helper r = run_case("expect_fails");
    expect_failed(r, "expect_fails");
    EXPECT_TRUE(has(r, where("isolated-expect-marker"))) << show(r);
    EXPECT_TRUE(has(r, "isolated-expect-marker")) << show(r);
    EXPECT_TRUE(has(r, "1 + 1")) << show(r);
}

TEST(IsolatedOutput, assert_failure_shows_file_line_and_message)
{
    Helper r = run_case("assert_fails");
    expect_failed(r, "assert_fails");
    EXPECT_TRUE(has(r, where("isolated-assert-marker"))) << show(r);
    EXPECT_TRUE(has(r, "isolated-assert-marker")) << show(r);
    EXPECT_FALSE(has(r, "not reached")) << show(r);
}

TEST(IsolatedOutput, hang_names_sigalrm)
{
    Helper r = run_case("hangs");
    expect_failed(r, "hangs");
    EXPECT_TRUE(has(r, "SIGALRM")) << show(r);
}

TEST(IsolatedOutput, crash_names_the_signal)
{
    Helper r = run_case("crashes");
    expect_failed(r, "crashes");
    EXPECT_TRUE(has(r, "SIGABRT")) << show(r);
}

TEST(IsolatedOutput, failure_before_a_crash_still_shows)
{
    Helper r = run_case("expect_fails_then_crashes");
    expect_failed(r, "expect_fails_then_crashes");
    EXPECT_TRUE(has(r, where("isolated-before-crash-marker"))) << show(r);
    EXPECT_TRUE(has(r, "isolated-before-crash-marker")) << show(r);
    EXPECT_TRUE(has(r, "SIGABRT")) << show(r);
}

TEST(IsolatedOutput, leak_fails_with_lsan_report)
{
#ifndef BLOT_TEST_LSAN
    GTEST_SKIP() << "LeakSanitizer runs in the child only in an ASan build";
#else
    Helper r = run_case("leaks");
    expect_failed(r, "leaks");
    EXPECT_TRUE(has(r, "ERROR: LeakSanitizer: detected memory leaks")) << show(r);
    EXPECT_TRUE(has(r, "leak_a_block")) << show(r);
#endif
}
