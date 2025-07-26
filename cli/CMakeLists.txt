enable_testing()

ADD_EXECUTABLE(blot
        main.cpp
)

TARGET_LINK_LIBRARIES(blot
        blot_a
        -lm
)
