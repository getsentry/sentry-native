cmake_minimum_required (VERSION 3.0)
project (Sentry-Native LANGUAGES C CXX ASM)

include(CMakeDependentOption)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LINUX TRUE)
endif()

OPTION(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
CMAKE_DEPENDENT_OPTION(WITH_CRASHPAD "Build and use the Crashpad integration" ON "APPLE OR WIN32" OFF)

if(WITH_CRASHPAD AND NOT APPLE AND NOT WIN32)
	message(WARNING "The Crashpad backend is currently only supported on macOS and Windows")
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

if(BUILD_SHARED_LIBS)
	add_definitions(-DSENTRY_BUILD_SHARED)
else()
	add_definitions(-DSENTRY_BUILD_STATIC)
endif()

FIND_PACKAGE(CURL)
if(CURL_FOUND)
	set(WITH_CURL TRUE)
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

if(WITH_CRASHPAD AND WIN32)
	# Disable duplicate-define warnings, as the crashpad build defines all the
	# "slim windows.h" defines, which sentry_boot also defines.
	add_definitions(/wd4005)
endif()

if(ANDROID)
	set(WITH_LIBUNWINDSTACK TRUE)
elseif(NOT WIN32)
	set(WITH_LIBBACKTRACE TRUE)
endif()

include_directories("include")
include_directories("src")

file(GLOB_RECURSE SENTRY_NATIVE_TEST_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
)

if(LINUX)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "pthread" "dl")
endif()

if(WITH_LIBUNWINDSTACK)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/libunwindstack-ndk/include)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/libunwindstack-ndk/cmake)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "unwindstack")
endif()

if(WIN32)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} "dbghelp.lib" "pathcch.lib")
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
	install(TARGETS "crashpad_handler")
endif()

# ===== sentry library =====

add_library("sentry" "vendor/mpack.c")
add_subdirectory(src)

set_target_properties("sentry" PROPERTIES PUBLIC_HEADER "include/sentry.h")
target_link_libraries("sentry" ${LINK_LIBRARIES})
install(TARGETS "sentry")

if(WITH_CRASHPAD)
	add_dependencies("sentry" "crashpad_handler")
endif()

# ===== tests =====

# compile tests separately and pass an extra preprocessor define so we can
# switch some internal modes for the unittests.
# TODO: get sources from "sentry"
get_target_property(SENTRY_NATIVE_ALL_SOURCES sentry SOURCES)
add_executable("sentry_tests" EXCLUDE_FROM_ALL
	${SENTRY_NATIVE_ALL_SOURCES}
	${SENTRY_NATIVE_TEST_SOURCES}
)
get_target_property(SENTRY_DEFS sentry COMPILE_DEFINITIONS)
target_compile_definitions("sentry_tests" PUBLIC SENTRY_UNITTEST ${SENTRY_DEFS})
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

add_executable("example_crashpad" EXCLUDE_FROM_ALL "./examples/example_crashpad.c")
target_link_libraries("example_crashpad" "sentry")
