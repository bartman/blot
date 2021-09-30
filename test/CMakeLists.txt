enable_testing()

# --- simple test

ADD_EXECUTABLE(test-simple
        test-simple.c
)

TARGET_LINK_LIBRARIES(test-simple
        blot
)

# --- animation test

ADD_EXECUTABLE(test-trig
        test-trig.c
)

TARGET_LINK_LIBRARIES(test-trig
        blot
        -lm
)
