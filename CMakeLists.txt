cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories("include")

set(SENTRY_NATIVE_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIBUNWINDSTACK_SOURCE_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/external)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/libunwindstack-ndk/include)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/libunwindstack-ndk/cmake)
list(APPEND LINK_LIBRARIES unwindstack)

file(GLOB_RECURSE SENTRY_NATIVE_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.cpp
)

set(SENTRY_NATIVE_TESTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tests)

file(GLOB_RECURSE SENTRY_TEST_SOURCES
    ${SENTRY_NATIVE_TESTS_ROOT}/*.c
    ${SENTRY_NATIVE_TESTS_ROOT}/*.cpp
)

add_library("sentry" SHARED
    ${SENTRY_NATIVE_SOURCES}
)
target_link_libraries("sentry" ${LINK_LIBRARIES})


### Tests
add_executable("test_sentry"
    ${SENTRY_NATIVE_SOURCES}
    ${SENTRY_TEST_SOURCES}
)

target_include_directories("test_sentry" PRIVATE
    "src"
)
target_compile_definitions("test_sentry" PRIVATE
    -D SENTRY_WITH_TESTS -D SENTRY_WITH_NULL_UNWINDER
)

target_link_libraries("test_sentry" ${LINK_LIBRARIES})
