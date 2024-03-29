CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(blot)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
#FIND_PACKAGE(libev REQUIRED)
FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GLIB2 REQUIRED glib-2.0>=2.44)
PKG_CHECK_MODULES(GIO REQUIRED gio-2.0>=2.44)
PKG_CHECK_MODULES(GIOUNIX REQUIRED gio-unix-2.0>=2.44)
#PKG_CHECK_MODULES(GMODULE REQUIRED gmodule-2.0>=2.44)
#PKG_CHECK_MODULES(URIPARSER REQUIRED liburiparser>=0.8)
#PKG_CHECK_MODULES(MAGICCORE REQUIRED MagickCore>=6)
#PKG_CHECK_MODULES(MAGICWAND REQUIRED MagickWand>=6)

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_CXX_STANDARD 20)

add_definitions(-D_GNU_SOURCE)

if(CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-Wall -Werror -Wno-variadic-macros -Wno-dangling-pointer)
endif(CMAKE_COMPILER_IS_GNUCC)

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_REVISION)

add_definitions(-D"BLOT_GIT_REFSPEC=${GIT_REFSPEC}")
add_definitions(-D"BLOT_GIT_REVISION=${GIT_REVISION}")
add_definitions(-D"BLOT_GIT_SHA1=${GIT_SHA1}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(lib)
#add_subdirectory(cli)
add_subdirectory(examples)

enable_testing()
include(CTest)
add_subdirectory(tests)
