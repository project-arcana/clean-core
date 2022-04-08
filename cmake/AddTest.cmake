
# ===============================================
# Options

option(ARC_TESTS_ENABLE_ASAN "if true, enables clang/MSVC address sanitizer" OFF)
option(ARC_TESTS_ENABLE_MSAN "if true, enables clang/MSVC memory sanitizer" OFF)
option(ARC_TESTS_ENABLE_UBSAN "if true, enables clang/MSVC undefined behavior sanitizer" OFF)
option(ARC_TESTS_ENABLE_TSAN "if true, enables clang/MSVC thread sanitizer" OFF)

if (ARC_TESTS_ENABLE_ASAN AND ARC_TESTS_ENABLE_TSAN)
    message(FATAL_ERROR "Can only enable one of TSan or ASan at a time")
endif()
if (ARC_TESTS_ENABLE_ASAN AND ARC_TESTS_ENABLE_MSAN)
    message(FATAL_ERROR "Can only enable one of ASan or MSan at a time")
endif()

option(ARC_TESTS_DISABLE_EXCEPTIONS "Set compiler flags to disable exception handling" OFF)
option(ARC_TESTS_DISABLE_RTTI "Set compiler flags to disable RTTI" OFF)
option(ARC_TESTS_ENABLE_AVX2 "Set compiler flags to enable AVX2 instructions (and older ones included by it)" OFF)


# ===============================================
# Compile flags

set(ARC_TESTS_COMMON_COMPILER_FLAGS "")
set(ARC_TESTS_COMMON_LINKER_FLAGS "")

if (MSVC)
    list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS
        /MP
    )

    if (ARC_TESTS_ENABLE_AVX2)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS
            /arch:AVX2
        )
    endif()

    if (ARC_TESTS_DISABLE_EXCEPTIONS)
        string(REPLACE "/EHsc" "/EHs-c- /D _HAS_EXCEPTIONS=0" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif()

    if (ARC_TESTS_DISABLE_RTTI)
        string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif()
else()
    list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS
        -Wall
        -Wno-unused-variable
    )

    if (ARC_TESTS_ENABLE_AVX2)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS
            -mavx2
        )
    endif()

    if (ARC_TESTS_DISABLE_EXCEPTIONS)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS -fno-exceptions)
    endif()

    if (ARC_TESTS_DISABLE_RTTI)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS -fno-rtti)
    endif()

    if (ARC_TESTS_ENABLE_ASAN OR ARC_TESTS_ENABLE_TSAN OR ARC_TESTS_ENABLE_MSAN OR ARC_TESTS_ENABLE_UBSAN)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS -fno-omit-frame-pointer -g)
        list(APPEND ARC_TESTS_COMMON_LINKER_FLAGS -fno-omit-frame-pointer -g)
    endif()

    if (ARC_TESTS_ENABLE_ASAN)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS -fsanitize=address)
        list(APPEND ARC_TESTS_COMMON_LINKER_FLAGS -fsanitize=address)
    endif()

    if (ARC_TESTS_ENABLE_TSAN)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS -fsanitize=thread)
        list(APPEND ARC_TESTS_COMMON_LINKER_FLAGS -fsanitize=thread)
    endif()

    if (ARC_TESTS_ENABLE_MSAN)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS -fsanitize=memory)
        list(APPEND ARC_TESTS_COMMON_LINKER_FLAGS -fsanitize=memory)
    endif()

    if (ARC_TESTS_ENABLE_UBSAN)
        list(APPEND ARC_TESTS_COMMON_COMPILER_FLAGS
            -fsanitize=undefined
            -fno-sanitize-recover=all
            -fno-sanitize=alignment,vptr
        )
        list(APPEND ARC_TESTS_COMMON_LINKER_FLAGS
            -fsanitize=undefined
            -fno-sanitize-recover=all
            -fno-sanitize=alignment,vptr
        )
    endif()
endif()


# ===============================================
# Compile flags

function(add_arcana_test TEST_NAME SOURCES)
    # create target
    add_executable(${TEST_NAME} ${SOURCES})

    # set compiler flags
    target_compile_options(${TEST_NAME} PUBLIC ${ARC_TESTS_COMMON_COMPILER_FLAGS})

    # set linker flags, make nexus available
    target_link_libraries(${TEST_NAME} PUBLIC nexus ${ARC_TESTS_COMMON_LINKER_FLAGS})

    # move into tests folder
    set_property(TARGET ${TEST_NAME} PROPERTY FOLDER "Tests")
endfunction()
