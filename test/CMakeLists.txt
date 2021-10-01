enable_testing()

SET(TEST_EXECUTABLES
        test-line-spin
        test-simple
        test-trig
        test-trig-loop
        test-x
)

foreach(test ${TEST_EXECUTABLES})

        ADD_EXECUTABLE(${test}
                ${test}.c
        )

        TARGET_LINK_LIBRARIES(${test}
                blot
                -lm
        )

endforeach(test)
