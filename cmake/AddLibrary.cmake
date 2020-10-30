
# usage:
#   arcana_prepare_library(ML my-lib SOURCES HEADERS)
#
# - sets up source groups for sources and headers
# - enables unity builds for all sources (including an option to opt-out)
function(arcana_prepare_library LIB_PREFIX LIB_TARGET SOURCES_VARIABLE_NAME HEADERS_VARIABLE_NAME)

    if (CC_VERBOSE_CMAKE)
        message(STATUS "[${LIB_TARGET}] configuring library")
    endif()

    arcana_source_group(${SOURCES_VARIABLE_NAME} ${HEADERS_VARIABLE_NAME})

    option(${LIB_PREFIX}_ENABLE_UNITY_BUILD "If enabled, compiles this library as a single compilation unit" ON)

    if (${${LIB_PREFIX}_ENABLE_UNITY_BUILD})
        if (CC_VERBOSE_CMAKE)
            message(STATUS "[${LIB_TARGET}] enabling unity builds")
        endif()
        arcana_enable_unity_build(${LIB_TARGET} ${SOURCES_VARIABLE_NAME} 100 cc)
    endif()
endfunction()

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

    arcana_source_group(${SOURCES_VARIABLE_NAME} ${HEADERS_VARIABLE_NAME})

    option(${LIB_PREFIX}_ENABLE_UNITY_BUILD "If enabled, compiles this library as a single compilation unit" ON)

    if (${${LIB_PREFIX}_ENABLE_UNITY_BUILD})
        if (CC_VERBOSE_CMAKE)
            message(STATUS "[${LIB_TARGET}] enabling unity builds")
        endif()
        arcana_enable_unity_build(${LIB_TARGET} ${SOURCES_VARIABLE_NAME} 100 cc)
    endif()

    add_library(${LIB_TARGET} STATIC ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})

    arcana_configure_lib_options(${LIB_TARGET})

endfunction()

# same as arcana_add_library but without unity build
function(arcana_add_library_no_unity LIB_PREFIX LIB_TARGET SOURCES_VARIABLE_NAME HEADERS_VARIABLE_NAME)

    if (CC_VERBOSE_CMAKE)
        message(STATUS "[${LIB_TARGET}] configuring library")
    endif()

    arcana_source_group(${SOURCES_VARIABLE_NAME} ${HEADERS_VARIABLE_NAME})

    add_library(${LIB_TARGET} STATIC ${${SOURCES_VARIABLE_NAME}} ${${HEADERS_VARIABLE_NAME}})

    arcana_configure_lib_options(${LIB_TARGET})

endfunction()
