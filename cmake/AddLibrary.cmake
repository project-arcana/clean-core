
# usage:
#   arcana_add_library(ML my-lib SOURCES HEADERS)
#
#   (where ML is the prefix used in cmake options for my-lib)
#   (NOTE: sources and headers are variable names, do NOT use ${SOURCES} and ${HEADERS})
#
# - calls add_library
# - sets up source groups for sources and headers
# - enables unity builds for all sources (including an option to opt-out)
# - sets up compile flags (errors, warning level, etc.)
function(arcana_add_library LIB_PREFIX LIB_TARGET SOURCES_VARIABLE_NAME HEADERS_VARIABLE_NAME)

    if (CC_VERBOSE_CMAKE)
        message(STATUS "[${LIB_TARGET}] configuring library")
    endif()

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})

    option(${LIB_PREFIX}_ENABLE_UNITY_BUILD "If enabled, compiles this library as a single compilation unit" ON)
    option(${LIB_PREFIX}_BUILD_DLL "If enabled, build a shared DLL instead of a static library" OFF)

    if (${${LIB_PREFIX}_ENABLE_UNITY_BUILD})
        if (CC_VERBOSE_CMAKE)
            message(STATUS "[${LIB_TARGET}] enabling unity builds")
        endif()
        arcana_enable_unity_build(${LIB_TARGET} ${SOURCES_VARIABLE_NAME} 100 cc)
    endif()

    if (${${LIB_PREFIX}_BUILD_DLL})
        if (CC_VERBOSE_CMAKE)
            message(STATUS "[${LIB_TARGET}] building shared library (DLL)")
        endif()

        add_library(${LIB_TARGET} SHARED ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})
        # define <LIB>_BUILD_DLL always, and <LIB>_DLL only privately - when the DLL itself is being built
        # macro is used to differentiate between dllexport/dllimport
        target_compile_definitions(${LIB_TARGET} PUBLIC ${LIB_PREFIX}_BUILD_DLL PRIVATE ${LIB_PREFIX}_DLL)
    else()
        if (CC_VERBOSE_CMAKE)
            message(STATUS "[${LIB_TARGET}] building static library")
        endif()
        add_library(${LIB_TARGET} STATIC ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})
    endif()

    arcana_configure_lib_options(${LIB_TARGET})

endfunction()

# same as arcana_add_library but without unity build
function(arcana_add_library_no_unity LIB_PREFIX LIB_TARGET SOURCES_VARIABLE_NAME HEADERS_VARIABLE_NAME)

    if (CC_VERBOSE_CMAKE)
        message(STATUS "[${LIB_TARGET}] configuring library")
    endif()

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})

    add_library(${LIB_TARGET} STATIC ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})

    arcana_configure_lib_options(${LIB_TARGET})

endfunction()
