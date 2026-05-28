
# set up compile options we want for a library
function(arcana_configure_lib_options LIB_TARGET)
    if (MSVC)
        target_compile_options(${LIB_TARGET} PUBLIC
            # /MP is an MSVC-only flag; clang-cl reports it as an unused argument
            $<$<CXX_COMPILER_ID:MSVC>:/MP> # multi-threaded compilation
            /we4715 # error on missing return
            /we4477 # printf warnings as errors
            /we4474 # printf warnings as errors
        )
    else()
        target_compile_options(${LIB_TARGET} PRIVATE
            -Wall
            -fPIC
            -Werror=return-type # error on missing return
            -Werror=format # printf warnings as errors
        )
        if(LINUX)
            option(CC_LINKER_MOLD "If true, uses the mold linker (default, must be installed)" ON)
            option(CC_LINKER_GOLD "If true, uses the gold linker (must be installed)" OFF)
            if (CC_LINKER_MOLD)
                target_link_libraries(${LIB_TARGET} PUBLIC -fuse-ld=mold)
            elseif (CC_LINKER_GOLD)
                target_link_libraries(${LIB_TARGET} PUBLIC -fuse-ld=gold)
            endif()
        endif()

        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        else() # GCC
            target_compile_options(${LIB_TARGET} PRIVATE
                -Wno-sign-compare # disable signed/unsigned warnings
                $<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess> # disable memset warnings
            )
        endif()
    endif()

    # disable floating point contractions (will ruin a lot of code otherwise)
    if (MSVC)
        target_compile_options(${LIB_TARGET} PUBLIC /fp:precise)
    else()
        target_compile_options(${LIB_TARGET} PUBLIC -ffp-contract=off)
    endif()

    # strict mode enables some Werror-xyz errors (mainly used in deploy and CI)
    if (CC_STRICT)
        if (CC_VERBOSE_CMAKE)
            message(STATUS "[${LIB_TARGET}] enable strict mode (selective Werrors)")
        endif()
        if (MSVC)
            target_compile_definitions(${LIB_TARGET} PRIVATE
                /WX # treat linker warnings as errors
                /we4101 # unreferenced local variable
            )
        else()
            target_compile_options(${LIB_TARGET} PRIVATE
                # unused entities
                -Werror=unused-variable
                -Werror=unused-function

                -Werror=deprecated-declarations # no deprecate warnings
                -Werror=switch # unhandled switch statements
            )
            if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                target_compile_options(${LIB_TARGET} PRIVATE
                    # more unused entities
                    -Werror=unused-private-field
                    -Werror=unneeded-internal-declaration
                )
            else() # GCC
                target_compile_options(${LIB_TARGET} PRIVATE
                    # more unused entities
                    -Werror=unused-but-set-variable
                )
            endif()
        endif()
    endif()

    if (CC_ENABLE_ADDRESS_SANITIZER)
        if (MSVC)
            target_compile_options(${LIB_TARGET} PUBLIC /fsanitize=address)
        else()
            target_compile_options(${LIB_TARGET} PUBLIC -fsanitize=address)
        endif()
    endif()
endfunction()
