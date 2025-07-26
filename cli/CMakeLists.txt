INCLUDE(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_Declare(
    clipp
    GIT_REPOSITORY https://github.com/muellan/clipp.git
    GIT_TAG v1.2.3
)
FetchContent_MakeAvailable(spdlog clipp)


ADD_EXECUTABLE(blot
        main.cpp
)

TARGET_INCLUDE_DIRECTORIES(blot PRIVATE
        ${clipp_SOURCE_DIR}/include
)

TARGET_LINK_LIBRARIES(blot
        blot_a
        -lm
        spdlog::spdlog
)
