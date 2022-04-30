cmake_minimum_required(VERSION 3.11)

file(GLOB_RECURSE SOURCES
    "*.cc"
    "*.hh"
)

add_arcana_test(tests-clean-core "${SOURCES}")

target_link_libraries(tests-clean-core PUBLIC
    clean-core
    clean-ranges
    typed-geometry
    rich-log
    ctracer
)
