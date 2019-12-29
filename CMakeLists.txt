cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

find_package(CMocka REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

OPTION(WITH_ASAN_OPTION "Build sentry-native with address sanitizer" OFF)
if(WITH_ASAN_OPTION)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer")
	set(CMAKE_LD_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

OPTION(WITH_TSAN_OPTION "Build sentry-native with thread sanitizer" OFF)
if(WITH_TSAN_OPTION)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=thread -fno-omit-frame-pointer")
	set(CMAKE_LD_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

include_directories("include" ${CMOCKA_INCLUDE_DIR})

set(SENTRY_NATIVE_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SENTRY_EXTRA_LIBS)

FIND_PACKAGE(CURL)
if(CURL_FOUND)
	include_directories(${CURL_INCLUDE_DIR})
	set(LINK_LIBRARIES ${LINK_LIBRARIES} ${CURL_LIBRARIES})
endif()

file(GLOB SENTRY_NATIVE_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.cpp
    ${SENTRY_NATIVE_SOURCE_ROOT}/transports/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/transports/*.cpp
)

file(GLOB SENTRY_NATIVE_UNIX_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/unix/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/unix/*.cpp
)

file(GLOB SENTRY_NATIVE_WINDOWS_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/windows/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/windows/*.cpp
)

file(GLOB_RECURSE SENTRY_NATIVE_TEST_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
)

set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_SOURCES})
if(WIN32)
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
		${SENTRY_NATIVE_WINDOWS_SOURCES})
else()
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
		${SENTRY_NATIVE_UNIX_SOURCES})
endif()

add_library("sentry" SHARED
	${SENTRY_NATIVE_ALL_SOURCES}
)
target_link_libraries("sentry" ${LINK_LIBRARIES})

add_executable("sentry_tests"
	${SENTRY_NATIVE_TEST_SOURCES}
)
target_link_libraries("sentry_tests" "sentry" ${CMOCKA_LIBRARY})
