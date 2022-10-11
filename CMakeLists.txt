cmake_minimum_required(VERSION 3.8)
project(CleanCore)

# =========================================
# global options

option(CC_STRICT "if true, enables all warnings and -Werror" OFF)
option(CC_ENABLE_ASSERTIONS "if true, enables assertions (also in RelWithDebInfo)" ON)
option(CC_ENABLE_BOUND_CHECKING "if true, enables bound checking (e.g. for containers and iterators, only if assertions are active)" ON)
option(CC_ENABLE_NULL_CHECKING "if true, enables null checking (e.g. for smart pointers, only if assertions are active)" ON)
option(CC_ENABLE_CONTRACT_CHECKING "if true, enables contract checking (e.g. pre- and postconditions, only if assertions are active)" ON)
option(CC_VERBOSE_CMAKE "if true, adds more verbose cmake output (for debugging)" OFF)

# =========================================
# import scripts

include(cmake/UnityBuild.cmake)
include(cmake/SourceGroup.cmake)
include(cmake/CompileOptions.cmake)
include(cmake/AddLibrary.cmake)

# TODO: make configurable
include(cmake/AddTest.cmake)

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh" "src/*.inl")
file(GLOB_RECURSE MISC_FILES "*.natvis")

# set up source_group, optionally enable unity builds, add the library
arcana_add_library(CC clean-core SOURCES HEADERS)

# add natvis files (visual studio debug views for span, vector, etc.)
target_sources(clean-core PUBLIC ${MISC_FILES})

target_include_directories(clean-core PUBLIC src/)

# =========================================
# set up compile flags

# default to RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

target_link_libraries(clean-core PUBLIC
    $<$<PLATFORM_ID:Linux>:-pthread>
)

target_compile_definitions(clean-core PUBLIC
    $<$<CONFIG:DEBUG>:CC_DEBUG>
    $<$<CONFIG:RELEASE>:CC_RELEASE>
    $<$<CONFIG:RELWITHDEBINFO>:CC_RELWITHDEBINFO>
)

if (CC_ENABLE_ASSERTIONS)
    target_compile_definitions(clean-core PUBLIC $<$<CONFIG:DEBUG>:CC_ENABLE_ASSERTIONS>)
    target_compile_definitions(clean-core PUBLIC $<$<CONFIG:RELWITHDEBINFO>:CC_ENABLE_ASSERTIONS>)
endif()

if (CC_ENABLE_BOUND_CHECKING)
    target_compile_definitions(clean-core PUBLIC CC_ENABLE_BOUND_CHECKING)
endif()

if (CC_ENABLE_NULL_CHECKING)
    target_compile_definitions(clean-core PUBLIC CC_ENABLE_NULL_CHECKING)
endif()

if (CC_ENABLE_CONTRACT_CHECKING)
    target_compile_definitions(clean-core PUBLIC CC_ENABLE_CONTRACT_CHECKING)
endif()
