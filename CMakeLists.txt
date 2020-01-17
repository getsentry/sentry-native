cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types -Wall -fvisibility=hidden")
if(NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

OPTION(WITH_ASAN_OPTION "Build sentry-native with address sanitizer" OFF)
if(WITH_ASAN_OPTION)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer")
	link_libraries("-fsanitize=address")
endif()

OPTION(WITH_TSAN_OPTION "Build sentry-native with thread sanitizer" OFF)
if(WITH_TSAN_OPTION)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=thread -fno-omit-frame-pointer")
	link_libraries("-fsanitize=thread")
endif()

# to fix some issues with tests (dladdr can't find functions otherwise)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--build-id=uuid,-E")
endif()

include_directories("include")

set(SENTRY_NATIVE_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SENTRY_EXTRA_LIBS)

add_definitions(-DSENTRY_BUILD_SHARED)

FIND_PACKAGE(CURL)
if(CURL_FOUND)
	add_definitions(-DSENTRY_WITH_LIBCURL_TRANSPORT)
	include_directories(${CURL_INCLUDE_DIR})
	set(LINK_LIBRARIES ${LINK_LIBRARIES} ${CURL_LIBRARIES})
endif()

file(GLOB SENTRY_NATIVE_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/*.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/transports/sentry_disk_transport.c
    ${SENTRY_NATIVE_SOURCE_ROOT}/transports/sentry_function_transport.c
)

file(GLOB SENTRY_NATIVE_LIBCURL_TRANSPORT_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/transports/sentry_libcurl_transport.c
)

file(GLOB SENTRY_NATIVE_INPROC_BACKEND_SOURCES
	${SENTRY_NATIVE_SOURCE_ROOT}/backends/sentry_inproc_*.c
)

file(GLOB SENTRY_NATIVE_UNIX_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/unix/*.c
)

file(GLOB SENTRY_NATIVE_DARWIN_SOURCES
	${SENTRY_NATIVE_SOURCE_ROOT}/darwin/*.c
)

file(GLOB SENTRY_NATIVE_WINDOWS_SOURCES
    ${SENTRY_NATIVE_SOURCE_ROOT}/windows/*.c
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
		${SENTRY_NATIVE_UNIX_SOURCES} ${SENTRY_NATIVE_INPROC_BACKEND_SOURCES})
endif()

if(CURL_FOUND)
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
		${SENTRY_NATIVE_LIBCURL_TRANSPORT_SOURCES})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
		${SENTRY_NATIVE_DARWIN_SOURCES})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "pthread" "dl")
endif()

if(WIN32)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "dbghelp.lib")
endif()

add_library("sentry" SHARED
	${SENTRY_NATIVE_ALL_SOURCES}
)
target_link_libraries("sentry" ${LINK_LIBRARIES})

# compile tests separately and pass an extra preprocessor define so we can
# switch some internal modes for the unittests.
add_executable("sentry_tests"
	${SENTRY_NATIVE_ALL_SOURCES}
	${SENTRY_NATIVE_TEST_SOURCES}
)
target_compile_definitions("sentry_tests" PUBLIC -DSENTRY_UNITTEST)
target_link_libraries("sentry_tests" ${LINK_LIBRARIES})

add_executable("example" "./examples/example.c")
target_link_libraries("example" "sentry")