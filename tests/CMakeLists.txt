cmake_minimum_required(VERSION 3.11)

file(GLOB_RECURSE SOURCES
    "*.cc"
    "*.hh"
)

add_arcana_test(tests-clean-core "${SOURCES}")

target_link_libraries(tests-clean-core PUBLIC
    clean-core
    typed-geometry
    rich-log
)

if (TARGET clean-ranges)
	target_link_libraries(tests-clean-core PUBLIC clean-ranges)
	target_compile_definitions(tests-clean-core PUBLIC HAS_CLEAN_RANGES)
endif()

if (TARGET ctracer)
	target_link_libraries(tests-clean-core PUBLIC ctracer)
	target_compile_definitions(tests-clean-core PUBLIC HAS_CTRACER)
endif()
