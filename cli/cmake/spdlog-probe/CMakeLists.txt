# Probe for cli/CMakeLists.txt: does the system spdlog (and the fmt it was
# built against) compile with this compiler?  A project of its own, built
# by try_compile, so its find_package targets don't leak into the real
# build when the answer is no and the cli falls back to FetchContent.
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(blot_spdlog_probe LANGUAGES CXX)

find_package(spdlog REQUIRED)

# compile only: the failure being probed is in the headers
ADD_LIBRARY(probe STATIC probe.cpp)
TARGET_LINK_LIBRARIES(probe PRIVATE spdlog::spdlog)
