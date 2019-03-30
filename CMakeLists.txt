cmake_minimum_required(VERSION 3.8)
project(CleanCore)

option(CC_STRICT "if true, enables all warnings and -Werror")

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")
file(GLOB_RECURSE HEADERS "include/cc/*")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" FILES ${HEADERS})

add_library(clean-core STATIC ${SOURCES} ${HEADERS})
target_include_directories(clean-core PUBLIC include/)

if (MSVC)
    target_compile_options(clean-core PUBLIC /MP)
else()
    target_compile_options(clean-core PRIVATE -Wall -fPIC)
    target_link_libraries(clean-core PUBLIC -fuse-ld=gold)
endif()

if (CC_STRICT)
    if (MSVC)
        # TODO
    else()
        target_compile_options(clean-core PRIVATE -Wextra -Wpedantic -Werror)
    endif()
endif()
