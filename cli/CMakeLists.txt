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
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
)
FetchContent_MakeAvailable(spdlog clipp fmt)

ADD_EXECUTABLE(blot
    main.cpp
    config.cpp
    reader.cpp
)

TARGET_COMPILE_DEFINITIONS(blot PRIVATE
    FMT_HEADER_ONLY
)

TARGET_INCLUDE_DIRECTORIES(blot PRIVATE
    ${clipp_SOURCE_DIR}/include
)

TARGET_LINK_LIBRARIES(blot PRIVATE
    blot_a
    -lm
    spdlog::spdlog
    fmt::fmt
)
