enable_testing()

# --- protocol testing

ADD_EXECUTABLE(test-simple
        test-simple.c
)

TARGET_LINK_LIBRARIES(test-simple
        blot
)
