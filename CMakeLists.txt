cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

find_package(CMocka REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories("include" ${CMOCKA_INCLUDE_DIR})

set(SENTRY_NATIVE_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SENTRY_NATIVE_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.cpp
)

file(GLOB_RECURSE SENTRY_NATIVE_TEST_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
)

add_library("sentry" SHARED
    ${SENTRY_NATIVE_SOURCES}
)
target_link_libraries("sentry" ${LINK_LIBRARIES})

add_executable("sentry_tests"
	${SENTRY_NATIVE_TEST_SOURCES}
)
target_link_libraries("sentry_tests" "sentry" ${CMOCKA_LIBRARY})
