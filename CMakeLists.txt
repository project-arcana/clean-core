cmake_minimum_required(VERSION 3.8)
project(CleanCore)

# =========================================
# global options

option(CC_STRICT "if true, enables all warnings and -Werror" ON)
option(CC_ENABLE_ASSERTIONS "if true, enables assertions (even in release)" ON)
option(CC_ENABLE_BOUND_CHECKING "if true, enables bound checking (e.g. for containers and iterators, even in release)" ON)
option(CC_ENABLE_NULL_CHECKING "if true, enables null checking (e.g. for smart pointers, even in release)" ON)
option(CC_ENABLE_CONTRACT_CHECKING "if true, enables contract checking (e.g. pre- and postconditions, even in release)" ON)

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")
file(GLOB_RECURSE HEADERS "include/cc/*")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" FILES ${HEADERS})

add_library(clean-core STATIC ${SOURCES} ${HEADERS})
target_include_directories(clean-core PUBLIC include/)

# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(clean-core PUBLIC /MP)
else()
    target_compile_options(clean-core PRIVATE -Wall -fPIC)
    target_link_libraries(clean-core PUBLIC -fuse-ld=gold)
endif()

if (CC_ENABLE_ASSERTIONS)
    target_compile_definitions(clean-core PUBLIC CC_ENABLE_ASSERTIONS)
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
        # TODO
    else()
        target_compile_options(clean-core PRIVATE -Wextra -Wpedantic -Werror)
    endif()
endif()
