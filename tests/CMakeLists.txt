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

# RUN_ISOLATED bodies that fail on purpose (#26): built, not a test of its
# own; test_isolated runs it and checks what the parent shows
add_executable(isolated_failing isolated_failing.cpp)
target_link_libraries(isolated_failing GTest::gtest_main pthread blot_so)
add_dependencies(test_isolated isolated_failing)
target_compile_definitions(test_isolated PRIVATE
    BLOT_ISOLATED_HELPER="$<TARGET_FILE:isolated_failing>"
    BLOT_ISOLATED_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/isolated_failing.cpp")
