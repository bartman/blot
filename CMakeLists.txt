CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(blot VERSION 0.0.6 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
#FIND_PACKAGE(libev REQUIRED)
FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GLIB2 REQUIRED glib-2.0>=2.44)
#PKG_CHECK_MODULES(GIO REQUIRED gio-2.0>=2.44)
#PKG_CHECK_MODULES(GIOUNIX REQUIRED gio-unix-2.0>=2.44)

LINK_DIRECTORIES(
    ${GLIB2_LIBRARY_DIRS}
    ${GIO_LIBRARY_DIRS}
    ${GIOUNIX_LIBRARY_DIRS}
)

if (NOT DEFINED CMAKE_C_STANDARD)
    SET(CMAKE_C_STANDARD 99) # use C99 if one is not set by user
endif()
if (NOT DEFINED CMAKE_CXX_STANDARD)
    SET(CMAKE_CXX_STANDARD 20) # used only in test harness
endif()

add_definitions(-D_GNU_SOURCE)

if(CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-Wall -Werror -Wno-variadic-macros -Wno-dangling-pointer)
endif(CMAKE_COMPILER_IS_GNUCC)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "ASAN enabled for debug build")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG=1 -gdwarf-3 -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -gdwarf-3 -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
else()
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNDEBUG=1")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG=1")
endif()

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_REVISION)

add_definitions(-D"BLOT_GIT_REFSPEC=${GIT_REFSPEC}")
add_definitions(-D"BLOT_GIT_REVISION=${GIT_REVISION}")
add_definitions(-D"BLOT_GIT_SHA1=${GIT_SHA1}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(lib)
#add_subdirectory(cli)
add_subdirectory(examples/c)
add_subdirectory(examples/cpp)

enable_testing()
include(CTest)
add_subdirectory(tests)
