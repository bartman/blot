enable_testing()

SET(TEST_EXECUTABLES
        c-histogram
        c-line-spin
        c-simple
        c-trig
        c-trig-loop
        c-x
)

foreach(test ${TEST_EXECUTABLES})

        ADD_EXECUTABLE(${test}
                ${test}.c
        )

        TARGET_LINK_LIBRARIES(${test}
                blot_a
                -lm
        )

endforeach(test)
