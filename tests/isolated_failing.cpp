/* blot: RUN_ISOLATED bodies that fail on purpose (#26)
 *
 * Not a test of its own: the name doesn't match test_*.cpp, so
 * tests/CMakeLists.txt builds it but doesn't add it to ctest.
 * test_isolated.cpp runs one case at a time and checks that the parent's
 * output shows why the child failed.  Each failing line carries a marker
 * ("isolated-...-marker") that test_isolated.cpp looks up in this file
 * to know the line the output must name. */
#include "blot_test.hpp"

#include <csignal>
#include <cstdlib>
#include <thread>

#include <sys/prctl.h>
#include <sys/resource.h>

namespace {

/* not inlined, so no pointer to the block stays in the caller's frame */
/* abort() without a core dump on every ctest run: RLIMIT_CORE 0 for a
 * file core_pattern; a piped one (systemd-coredump) still gets the dump
 * unless the process isn't dumpable */
void no_core()
{
    struct rlimit rl = { 0, 0 };
    setrlimit(RLIMIT_CORE, &rl);
    prctl(PR_SET_DUMPABLE, 0);
}

__attribute__((noinline)) void leak_a_block()
{
    void *volatile p = malloc(48);
    (void)p;
}

} // namespace

TEST(Isolated, expect_fails)
{
    RUN_ISOLATED(5, {
        EXPECT_EQ(1 + 1, 3) << "isolated-expect-marker";
        if (!HasFailure())
            ADD_FAILURE() << "HasFailure() missed the failure";
    });
}

TEST(Isolated, assert_fails)
{
    RUN_ISOLATED(5, {
        ASSERT_TRUE(1 + 1 == 3) << "isolated-assert-marker";
        ADD_FAILURE() << "not reached: ASSERT returns from the body";
    });
}

TEST(Isolated, hangs)
{
    RUN_ISOLATED(1, {
        for (;;)
            pause();
    });
}

TEST(Isolated, crashes)
{
    RUN_ISOLATED(5, {
        no_core();
        abort();
    });
}

/* the failure must show even though the child never gets to the end */
TEST(Isolated, expect_fails_then_crashes)
{
    RUN_ISOLATED(5, {
        no_core();
        EXPECT_EQ(2 + 2, 5) << "isolated-before-crash-marker";
        abort();
    });
}

/* a failure on another thread: the body's reporter doesn't see it, the
 * end of the child writes it */
TEST(Isolated, thread_fails)
{
    RUN_ISOLATED(5, {
        std::thread t([] { ADD_FAILURE() << "isolated-thread-marker"; });
        t.join();
    });
}

/* a leak fails the child only in an ASan build (BLOT_TEST_LSAN) */
TEST(Isolated, leaks)
{
    RUN_ISOLATED(5, {
        leak_a_block();
    });
}
