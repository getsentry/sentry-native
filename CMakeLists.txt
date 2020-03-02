cmake_minimum_required (VERSION 3.10)
project (Sentry-Native LANGUAGES C CXX ASM)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LINUX TRUE)
endif()

if(APPLE OR WIN32)
	set(SENTRY_DEFAULT_BACKEND "crashpad")
else()
	set(SENTRY_DEFAULT_BACKEND "inproc")
endif()

OPTION(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(SENTRY_BACKEND ${SENTRY_DEFAULT_BACKEND} CACHE STRING
  "The sentry backend responsible for reporting crashes, can be either 'none', 'inproc' or 'crashpad'.")

if(SENTRY_BACKEND STREQUAL "crashpad")
	set(BACKEND_CRASHPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "inproc")
	set(BACKEND_INPROC TRUE)
elseif(SENTRY_BACKEND STREQUAL "none")
	set(BACKEND_NONE TRUE)
else()
	message(FATAL_ERROR "SENTRY_BACKEND must be one of 'none', 'inproc' or 'crashpad'")
endif()

if(BACKEND_CRASHPAD AND NOT APPLE AND NOT WIN32)
	message(FATAL_ERROR "The Crashpad backend is currently only supported on macOS and Windows")
endif()
if(BACKEND_INPROC AND WIN32)
	message(FATAL_ERROR "The in-process backend is not supported on Windows")
endif()

# use -O3 when doing `RelWithDebInfo` builds
foreach(lang ASM C CXX)
	# unix-like syntax
	string(REPLACE "-O2" "-O3" CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}")
	# windows-like syntax
	string(REPLACE "/O2" "/O3" CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}")
endforeach()

# https://gitlab.kitware.com/cmake/cmake/issues/20256
if(APPLE)
	find_program(DSYMUTIL_PROGRAM dsymutil)
	if(DSYMUTIL_PROGRAM)
		foreach(lang C CXX)
			foreach(var LINK_EXECUTABLE CREATE_SHARED_LIBRARY)
				set(CMAKE_${lang}_${var} "${CMAKE_${lang}_${var}}" "${DSYMUTIL_PROGRAM} <TARGET>")
			endforeach()
		endforeach()
	endif()
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

if(BACKEND_CRASHPAD)
	set(CMAKE_CXX_STANDARD 14)
else()
	set(CMAKE_CXX_STANDARD 11)
endif()

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types -Wall -fvisibility=hidden")
if(NOT WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fvisibility=hidden")
endif()

if(BACKEND_CRASHPAD AND WIN32)
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
	${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.c
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

# builtin `list(TRANSFORM)` (https://cmake.org/cmake/help/latest/command/list.html#transform)
# was only added in cmake 3.12, but we target 3.10, so we define a custom
# functions which can prepend a prefix to all list elements.
# copied from https://stackoverflow.com/a/27630120
FUNCTION(PREPEND var prefix)
	SET(listVar "")
	FOREACH(f ${ARGN})
		LIST(APPEND listVar "${prefix}/${f}")
	ENDFOREACH(f)
	SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

if(BACKEND_CRASHPAD)
	add_subdirectory(external/crashpad EXCLUDE_FROM_ALL)
	include_directories(external/crashpad external/crashpad/third_party/mini_chromium/mini_chromium)
	add_definitions(-DSENTRY_WITH_CRASHPAD_BACKEND)
	set(LINK_LIBRARIES ${LINK_LIBRARIES} crashpad_client crashpad_util)
	install(TARGETS "crashpad_handler")
elseif(BACKEND_INPROC)
	add_definitions(-DSENTRY_WITH_INPROC_BACKEND)
endif()

# ===== sentry library =====

add_library("sentry" "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mpack.c")
add_subdirectory(src)

set_target_properties("sentry" PROPERTIES PUBLIC_HEADER "include/sentry.h")
target_link_libraries("sentry" ${LINK_LIBRARIES})
install(TARGETS "sentry"
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include)

# https://gitlab.kitware.com/cmake/cmake/issues/18393
if(BUILD_SHARED_LIBS)
	if(APPLE)
		install(FILES "$<TARGET_FILE:sentry>.dSYM" DESTINATION lib)
	elseif(WIN32)
		install(FILES $<TARGET_PDB_FILE:sentry> DESTINATION lib)
	endif()
endif()

if(BACKEND_CRASHPAD)
	add_dependencies("sentry" "crashpad_handler")
endif()

# ===== tests =====

# compile tests separately and pass an extra preprocessor define so we can
# switch some internal modes for the unittests.
get_target_property(SENTRY_NATIVE_ALL_SOURCES sentry SOURCES)
add_executable("sentry_test_unit" EXCLUDE_FROM_ALL
	${SENTRY_NATIVE_ALL_SOURCES}
	${SENTRY_NATIVE_TEST_SOURCES}
)
get_target_property(SENTRY_DEFS sentry COMPILE_DEFINITIONS)
target_compile_definitions("sentry_test_unit" PUBLIC SENTRY_UNITTEST ${SENTRY_DEFS})
target_link_libraries("sentry_test_unit" ${LINK_LIBRARIES})

# to fix some issues with tests (dladdr can't find functions otherwise)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
	target_link_libraries("sentry_test_unit" "-Wl,--build-id=sha1,-E")
endif()

if(BACKEND_CRASHPAD)
	add_dependencies("sentry_test_unit" "crashpad_handler")
endif()

add_executable("sentry_test_integration" EXCLUDE_FROM_ALL "./tests/test_integration.c")
target_link_libraries("sentry_test_integration" "sentry")

# ===== examples =====

add_executable("example" EXCLUDE_FROM_ALL "./examples/example.c")
target_link_libraries("example" "sentry")

add_executable("example_crashpad" EXCLUDE_FROM_ALL "./examples/example_crashpad.c")
target_link_libraries("example_crashpad" "sentry")
