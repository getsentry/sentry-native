cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

include(CMakeDependentOption)

OPTION(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
CMAKE_DEPENDENT_OPTION(WITH_CRASHPAD "Build and use the Crashpad integration" ON "APPLE" OFF)

if(WITH_CRASHPAD AND NOT APPLE)
	message(WARNING "The Crashpad backend is currently only supported on macOS")
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

include_directories("include")

set(SENTRY_NATIVE_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(BUILD_SHARED_LIBS)
	add_definitions(-DSENTRY_BUILD_SHARED)
else()
	add_definitions(-DSENTRY_BUILD_STATIC)
endif()

FIND_PACKAGE(CURL)
if(CURL_FOUND)
	add_definitions(-DSENTRY_WITH_LIBCURL_TRANSPORT)
	include_directories(${CURL_INCLUDE_DIR})
	set(LINK_LIBRARIES ${LINK_LIBRARIES} ${CURL_LIBRARIES})
endif()

if(WITH_CRASHPAD)
	set(CMAKE_CXX_STANDARD 14)
else()
	set(CMAKE_CXX_STANDARD 11)
endif()

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types -Wall -fvisibility=hidden")
if(NOT WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fvisibility=hidden")
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

file(GLOB SENTRY_NATIVE_LINUX_SOURCES
	${SENTRY_NATIVE_SOURCE_ROOT}/linux/*.c
)

file(GLOB SENTRY_NATIVE_DARWIN_SOURCES
	${SENTRY_NATIVE_SOURCE_ROOT}/darwin/*.c
)

file(GLOB SENTRY_NATIVE_ANDROID_SOURCES
	${SENTRY_NATIVE_SOURCE_ROOT}/linux/sentry_procmaps_modulefinder.c
	${SENTRY_NATIVE_SOURCE_ROOT}/unix/sentry_unix_unwinder_libunwindstack.cpp
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
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
		${SENTRY_NATIVE_LINUX_SOURCES})

	set(LINK_LIBRARIES ${LINK_LIBRARIES} "pthread" "dl")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
			${SENTRY_NATIVE_ANDROID_SOURCES})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/libunwindstack-ndk/include)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/libunwindstack-ndk/cmake)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "unwindstack")
endif()

if(WIN32)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "dbghelp.lib")
endif()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})

if(WITH_CRASHPAD)
	add_subdirectory(external/crashpad EXCLUDE_FROM_ALL)
	include_directories(external/crashpad external/crashpad/third_party/mini_chromium/mini_chromium)
	add_definitions(-DSENTRY_WITH_CRASHPAD_BACKEND)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} crashpad_client crashpad_util)
	set(SENTRY_NATIVE_ALL_SOURCES ${SENTRY_NATIVE_ALL_SOURCES}
		${SENTRY_NATIVE_SOURCE_ROOT}/backends/sentry_crashpad_backend.cpp)
	install(TARGETS "crashpad_handler")
endif()

# ===== sentry library =====

add_library("sentry" ${SENTRY_NATIVE_ALL_SOURCES})
set_target_properties("sentry" PROPERTIES PUBLIC_HEADER "include/sentry.h")
target_link_libraries("sentry" ${LINK_LIBRARIES})
install(TARGETS "sentry")

if(WITH_CRASHPAD)
	add_dependencies("sentry" "crashpad_handler")
endif()

# ===== tests =====

# compile tests separately and pass an extra preprocessor define so we can
# switch some internal modes for the unittests.
add_executable("sentry_tests" EXCLUDE_FROM_ALL
	${SENTRY_NATIVE_ALL_SOURCES}
	${SENTRY_NATIVE_TEST_SOURCES}
)
target_compile_definitions("sentry_tests" PUBLIC -DSENTRY_UNITTEST)
target_link_libraries("sentry_tests" ${LINK_LIBRARIES})

# to fix some issues with tests (dladdr can't find functions otherwise)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
	target_link_libraries("sentry_tests" "-Wl,--build-id=sha1,-E")
endif()

if(WITH_CRASHPAD)
	add_dependencies("sentry_tests" "crashpad_handler")
endif()

# ===== examples =====

add_executable("example" EXCLUDE_FROM_ALL "./examples/example.c")
target_link_libraries("example" "sentry")
