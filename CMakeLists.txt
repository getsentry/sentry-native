cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# set(CMAKE_TOOLCHAIN_FILE $ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake)

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

add_definitions(-DSENTRY_WITH_INPROC_BACKEND)
add_library("sentry" SHARED
    ${SENTRY_NATIVE_SOURCES}
)
target_link_libraries("sentry" ${LINK_LIBRARIES})
