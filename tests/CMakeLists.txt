find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

message("GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS}")
message("GTEST_LIBRARIES ${GTEST_LIBRARIES}")

add_executable(test_init test_init.cpp)
target_link_libraries(test_init GTest::gtest_main pthread blot_so -lm)
add_test(NAME test_init COMMAND test_init)

