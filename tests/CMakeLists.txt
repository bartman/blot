find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

message("GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS}")
message("GTEST_LIBRARIES ${GTEST_LIBRARIES}")

file(GLOB TEST_SOURCES "test_*.cpp")

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
