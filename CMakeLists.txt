CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(blot VERSION 0.0.9 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

OPTION(ENABLE_ASAN "Enable AddressSanitizer" OFF)
IF(ENABLE_ASAN)
    MESSAGE(STATUS "AddressSanitizer enabled")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
ENDIF()

if (NOT DEFINED CMAKE_C_STANDARD)
    SET(CMAKE_C_STANDARD 99)
endif()
if (NOT DEFINED CMAKE_CXX_STANDARD)
    SET(CMAKE_CXX_STANDARD 20)
endif()

add_definitions(-D_GNU_SOURCE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Werror -Wno-variadic-macros -Wno-dangling-pointer)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Werror -Wno-variadic-macros)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG=1 -gdwarf-3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -gdwarf-3")
else()
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNDEBUG=1")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG=1")
endif()

include(CheckIPOSupported)
option(ENABLE_IPO "Enable IPO" ON)

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_REVISION)

add_definitions(-DBLOT_GIT_REFSPEC="${GIT_REFSPEC}")
add_definitions(-DBLOT_GIT_REVISION="${GIT_REVISION}")
add_definitions(-DBLOT_GIT_SHA1="${GIT_SHA1}")

#INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(cli)
add_subdirectory(examples/c)
add_subdirectory(examples/cpp)

enable_testing()
include(CTest)
add_subdirectory(tests)
