find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIBRARY_DIRS})

file(GLOB TEST_SOURCES "test_*.cpp")

# a --coverage build: tests that fork (RUN_ISOLATED in blot_test.hpp)
# write the child's counters too
if(CMAKE_C_FLAGS MATCHES "--coverage" OR CMAKE_CXX_FLAGS MATCHES "--coverage")
    add_compile_definitions(BLOT_TEST_GCOV=1)
endif()

foreach(TEST_SOURCE ${TEST_SOURCES})
    # Extract the filename without the extension to use as the test name
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # Create an executable for each test
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # Link libraries
    target_link_libraries(${TEST_NAME} GTest::gtest_main pthread blot_so)

    # Add a test
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# the CLI's plot encoder (#40) is a header in cli/
target_include_directories(test_cli_encode PRIVATE ${PROJECT_SOURCE_DIR}/cli)

# RUN_ISOLATED bodies that fail on purpose (#26): built, not a test of its
# own; test_isolated runs it and checks what the parent shows
add_executable(isolated_failing isolated_failing.cpp)
target_link_libraries(isolated_failing GTest::gtest_main pthread blot_so)
add_dependencies(test_isolated isolated_failing)
target_compile_definitions(test_isolated PRIVATE
    BLOT_ISOLATED_HELPER="$<TARGET_FILE:isolated_failing>"
    BLOT_ISOLATED_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/isolated_failing.cpp")

# the blot CLI without a terminal (#37): stdout a pipe, COLUMNS and LINES
# unset; it must not abort (std::terminate).  repeat: a plot to a file or
# a pipe is the same and complete every time, in any locale; write_error:
# a failed write is exit code 1 (#40); color_*, no_color_env: --color,
# --no-color and NO_COLOR, on stdout and stderr; interval*, one_frame: --interval and the
# frames drawn (#44, #47)
foreach(CLI_CASE help render regex repeat write_error
        color_never no_color_env color_auto color_always color_bad
        stderr_no_color
        interval interval_bad one_frame)
    add_test(NAME cli_notty_${CLI_CASE}
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/cli_test.sh $<TARGET_FILE:blot> ${CLI_CASE})
endforeach()
