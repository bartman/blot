# The cli's libraries (#30).  spdlog and fmt come from the system when
# it has them: the CI images install libspdlog-dev and libfmt-dev
# (ci/common.packages), so CI downloads nothing.  FetchContent from GitHub
# is only the fallback, for machines without them.  Each configure says
# which one it took ("cli: ...").
#
# clipp isn't packaged by Debian: its single header, v1.2.3 from
# https://github.com/muellan/clipp (MIT, see third_party/clipp/LICENSE),
# is in third_party/clipp, used unless find_package finds clipp.

find_package(clipp QUIET)
IF (clipp_FOUND)
    MESSAGE(STATUS "cli: clipp: system (find_package)")
ELSE()
    MESSAGE(STATUS "cli: clipp: vendored (third_party/clipp)")
ENDIF()

# spdlog first: a system spdlog built with SPDLOG_FMT_EXTERNAL (Debian's)
# finds its fmt itself, and the fmt the cli uses must be that one
find_package(spdlog QUIET)
IF (spdlog_FOUND)
    find_package(fmt QUIET)
ENDIF()

INCLUDE(FetchContent)

SET(BLOT_FETCHED_FMT OFF)
IF (NOT fmt_FOUND)
    MESSAGE(STATUS "cli: fmt: not found, FetchContent 12.1.0 from GitHub")
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.1.0
    )
    FetchContent_MakeAvailable(fmt)
    SET(BLOT_FETCHED_FMT ON)
ELSE()
    MESSAGE(STATUS "cli: fmt: system ${fmt_VERSION} (find_package)")
ENDIF()

IF (NOT spdlog_FOUND)
    MESSAGE(STATUS "cli: spdlog: not found, FetchContent v1.17.0 from GitHub")
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(spdlog)
ELSE()
    MESSAGE(STATUS "cli: spdlog: system ${spdlog_VERSION} (find_package)")
ENDIF()

ADD_EXECUTABLE(blot
    main.cpp
    config.cpp
    reader.cpp
)

# the fetched fmt is used header-only, as before #30; a system fmt is
# the shared library the system spdlog was built against
IF (BLOT_FETCHED_FMT)
    TARGET_COMPILE_DEFINITIONS(blot PRIVATE
        FMT_HEADER_ONLY
    )
ENDIF()

IF (NOT clipp_FOUND)
    TARGET_INCLUDE_DIRECTORIES(blot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/clipp
    )
ENDIF()

TARGET_LINK_LIBRARIES(blot PRIVATE
    blot_a
    -lm
    spdlog::spdlog
    fmt::fmt
)
