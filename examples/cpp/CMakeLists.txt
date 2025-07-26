enable_testing()

SET(TEST_EXECUTABLES
        cpp-histogram
        cpp-line-spin
        cpp-simple
        cpp-trig
        cpp-trig-loop
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
