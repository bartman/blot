enable_testing()

SET(TEST_EXECUTABLES
        test-simple
        test-line-spin
        test-trig
        test-trig-loop
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
