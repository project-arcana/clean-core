
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
        if (MSVC)
            target_compile_definitions(clean-core PRIVATE
                /WX # treat linker warnings as errors
                /we4101 # unreferenced local variable
            )
        else()
            target_compile_options(clean-core PRIVATE
                # unused entities
                -Werror=unused-variable
                -Werror=unused-function
                -Werror=unused-private-field
                -Werror=unneeded-internal-declaration

                -Werror=deprecated-declarations # no deprecate warnings
                -Werror=switch # unhandled switch statements
            )
        endif()
    endif()
endfunction()
