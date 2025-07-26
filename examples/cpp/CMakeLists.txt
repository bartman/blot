enable_testing()

SET(TEST_EXECUTABLES
        cpp-x
)

foreach(test ${TEST_EXECUTABLES})

        ADD_EXECUTABLE(${test}
                ${test}.cpp
        )

        TARGET_LINK_LIBRARIES(${test}
                blot_a
                -lm
        )

endforeach(test)
