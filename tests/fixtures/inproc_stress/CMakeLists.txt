cmake_minimum_required(VERSION 3.10)
project(inproc_stress_test C)

# SENTRY_LIB_DIR and SENTRY_INCLUDE_DIR are passed from the pytest
add_executable(inproc_stress_test
    main.c
    concurrent_crash.c
)

target_include_directories(inproc_stress_test PRIVATE ${SENTRY_INCLUDE_DIR})
target_link_directories(inproc_stress_test PRIVATE ${SENTRY_LIB_DIR})
target_link_libraries(inproc_stress_test PRIVATE sentry)

set_target_properties(inproc_stress_test PROPERTIES
    BUILD_RPATH "${SENTRY_LIB_DIR}"
)

if(NOT MSVC)
    target_compile_options(inproc_stress_test PRIVATE -O0 -g)
    target_link_libraries(inproc_stress_test PRIVATE pthread)
endif()
