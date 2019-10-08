cmake_minimum_required(VERSION 3.8)
project(CleanCore)

# =========================================
# global options

option(CC_STRICT "if true, enables all warnings and -Werror" OFF)
option(CC_ENABLE_ASSERTIONS "if true, enables assertions (also in RelWithDebInfo)" ON)
option(CC_ENABLE_BOUND_CHECKING "if true, enables bound checking (e.g. for containers and iterators, only if assertions are active)" ON)
option(CC_ENABLE_NULL_CHECKING "if true, enables null checking (e.g. for smart pointers, only if assertions are active)" ON)
option(CC_ENABLE_CONTRACT_CHECKING "if true, enables contract checking (e.g. pre- and postconditions, only if assertions are active)" ON)

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})

add_library(clean-core STATIC ${SOURCES})
target_include_directories(clean-core PUBLIC src/)

# =========================================
# set up compile flags

# default to RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if (MSVC)
    target_compile_options(clean-core PUBLIC /MP)
else()
    target_compile_options(clean-core PRIVATE -Wall -fPIC)
    target_link_libraries(clean-core PUBLIC -fuse-ld=gold)
endif()

target_compile_definitions(clean-core PUBLIC $<$<CONFIG:DEBUG>:CC_DEBUG>)
target_compile_definitions(clean-core PUBLIC $<$<CONFIG:RELEASE>:CC_RELEASE>)
target_compile_definitions(clean-core PUBLIC $<$<CONFIG:RELWITHDEBINFO>:CC_RELWITHDEBINFO>)

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

if (CC_STRICT)
    if (MSVC)
        target_compile_definitions(clean-core PRIVATE /WX /W4)
    else()
        target_compile_options(clean-core PRIVATE -Wextra -Wpedantic -Werror)
    endif()
endif()
