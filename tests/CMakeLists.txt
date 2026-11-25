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

# the blot CLI without a terminal (#37): stdout a pipe, COLUMNS and LINES
# unset; it must not abort (std::terminate).  repeat: a plot to a file or
# a pipe is the same and complete every time, in any locale (#40)
foreach(CLI_CASE help render regex repeat)
    add_test(NAME cli_notty_${CLI_CASE}
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/cli_test.sh $<TARGET_FILE:blot> ${CLI_CASE})
endforeach()
