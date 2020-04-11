
# set up compile options we want for a library
function(arcana_configure_lib_options LIB_TARGET)
    if (MSVC)
        target_compile_options(${LIB_TARGET} PUBLIC 
            /MP # multi-threaded compilation
            /we4715 # error on missing return
        )
    else()
        target_compile_options(${LIB_TARGET} PRIVATE
            -Wall
            -fPIC
            -Werror=return-type # error on missing return
        )
        target_link_libraries(${LIB_TARGET} PUBLIC -fuse-ld=gold)
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
endfunction()
