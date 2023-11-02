
# properly groups sources for Qt Creator / Visual Studio
#
# usage:
#
# file(GLOB_RECURSE SOURCES "src/*.cc")
# file(GLOB_RECURSE HEADERS "src/*.hh" "src/*.inl")
# arcana_source_group(SOURCES HEADERS)
#
#
macro(arcana_source_group)
    message(STATUS "arcana_source_group: in ${CMAKE_CURRENT_SOURCE_DIR}")
    foreach(loop_var ${ARGN})

        if (${CMAKE_GENERATOR} MATCHES "Visual Studio" OR WIN32)
            source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${${loop_var}})
        else()
            source_group("${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${${loop_var}})
        endif()

    endforeach()
endmacro()
