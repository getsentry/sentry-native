if(WIN32)
	cmake_minimum_required (VERSION 3.18)

	# enables support for CMAKE_MSVC_RUNTIME_LIBRARY
	cmake_policy(SET CMP0091 NEW)
else()
	# The Android tools ship with this ancient version, which we need to support.
	cmake_minimum_required (VERSION 3.10)
	cmake_policy(SET CMP0077 NEW)
endif()

# Include all platform files
set(SENTRY_PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake/platform")
file(GLOB_RECURSE SENTRY_PLATFORM_FILES "${SENTRY_PLATFORM_DIR}/*.cmake")
foreach(SENTRY_PLATFORM_FILE ${SENTRY_PLATFORM_FILES})
	include(${SENTRY_PLATFORM_FILE})
endforeach()

sentry_pre_project_operations()

#read sentry-native version
file(READ "include/sentry.h" _SENTRY_HEADER_CONTENT)
string(REGEX MATCH "#define SENTRY_SDK_VERSION \"([0-9\.]+)\"" _SENTRY_VERSION_MATCH "${_SENTRY_HEADER_CONTENT}")
set(SENTRY_VERSION "${CMAKE_MATCH_1}")
unset(_SENTRY_HEADER_CONTENT)
unset(_SENTRY_VERSION_MATCH)

project(Sentry-Native
	LANGUAGES C CXX ASM
	VERSION ${SENTRY_VERSION}
)

set(SENTRY_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(SENTRY_MAIN_PROJECT ON)
endif()

# We have 4 source directories:
# * `CMAKE_SOURCE_DIR`: points to the directory of the top-level CMakeLists.txt of the main project.
# * `CMAKE_CURRENT_SOURCE_DIR`: points to the directory of any CMakeLists.txt in any subdirectories.
# * `PROJECT_SOURCE_DIR`: points to the directory of any CMakeLists.txt in any subdirectories that defined a `project`.
# * `SENTRY_SOURCE_DIR`: points to the directory of this `CMakeLists.txt` independent of whether it was added as a
#                        subdirectory in another project or whether we access it from one of our subdirectories.
set(SENTRY_SOURCE_DIR ${PROJECT_SOURCE_DIR})

if(NOT CMAKE_C_STANDARD)
	set(CMAKE_C_STANDARD 11)
endif()

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
endif()

sentry_platform_init()

include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/sentry")

#setup sentry library type
if(SENTRY_MAIN_PROJECT AND NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON)
endif()
option(SENTRY_BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ${BUILD_SHARED_LIBS})
if(SENTRY_BUILD_SHARED_LIBS)
	set(SENTRY_LIBRARY_TYPE SHARED)
else()
	set(SENTRY_LIBRARY_TYPE STATIC)
endif()

option(SENTRY_PIC "Build sentry (and dependent) libraries as position independent libraries" ON)

option(SENTRY_TRANSPORT_COMPRESSION "Enable transport gzip compression" OFF)

option(SENTRY_BUILD_TESTS "Build sentry-native tests" "${SENTRY_MAIN_PROJECT}")
option(SENTRY_BUILD_EXAMPLES "Build sentry-native example(s)" "${SENTRY_MAIN_PROJECT}")

# CMAKE_POSITION_INDEPENDENT_CODE must be set BEFORE adding any libraries (including subprojects)
if(SENTRY_PIC)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
	set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
endif()

set(SENTRY_TRANSPORT ${SENTRY_DEFAULT_TRANSPORT} CACHE STRING
	"The HTTP transport that sentry uses to submit events to the sentry server, can be either 'none', 'curl' or 'winhttp' on windows.")

if(SENTRY_TRANSPORT STREQUAL "winhttp")
	set(SENTRY_TRANSPORT_WINHTTP TRUE)
elseif(SENTRY_TRANSPORT STREQUAL "curl")
	set(SENTRY_TRANSPORT_CURL TRUE)
elseif(SENTRY_TRANSPORT STREQUAL "none")
	set(SENTRY_TRANSPORT_NONE TRUE)
else()
	message(FATAL_ERROR "SENTRY_TRANSPORT must be one of 'none', 'curl' or 'winhttp'")
endif()

if(SENTRY_TRANSPORT_WINHTTP AND NOT WIN32)
	message(FATAL_ERROR "The winhttp transport is only supported on Windows.")
endif()

if(SENTRY_BUILD_TESTS OR SENTRY_BUILD_EXAMPLES)
	enable_testing()
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
	set(SENTRY_MAIN_PROJECT ON)
endif()

option(SENTRY_ENABLE_INSTALL "Enable sentry installation" "${SENTRY_MAIN_PROJECT}")

if(NOT DEFINED SENTRY_BACKEND)
	set(SENTRY_BACKEND ${SENTRY_DEFAULT_BACKEND} CACHE STRING
		"The sentry backend responsible for reporting crashes, can be either 'none', 'inproc', 'breakpad' or 'crashpad'.")
endif()

if(SENTRY_BACKEND STREQUAL "crashpad")
	set(SENTRY_BACKEND_CRASHPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "inproc")
	set(SENTRY_BACKEND_INPROC TRUE)
elseif(SENTRY_BACKEND STREQUAL "breakpad")
	set(SENTRY_BACKEND_BREAKPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "none")
	set(SENTRY_BACKEND_NONE TRUE)
else()
	message(FATAL_ERROR "SENTRY_BACKEND must be one of 'crashpad', 'inproc', 'breakpad' or 'none'")
endif()

set(SENTRY_SDK_NAME "" CACHE STRING "The SDK name to report when sending events.")

message(STATUS "SENTRY_TRANSPORT=${SENTRY_TRANSPORT}")
message(STATUS "SENTRY_BACKEND=${SENTRY_BACKEND}")
message(STATUS "SENTRY_LIBRARY_TYPE=${SENTRY_LIBRARY_TYPE}")
message(STATUS "SENTRY_SDK_NAME=${SENTRY_SDK_NAME}")

option(WITH_ASAN_OPTION "Build sentry-native with address sanitizer" OFF)
if(WITH_ASAN_OPTION)
	add_compile_options(-g -fsanitize=address -fno-omit-frame-pointer)
	link_libraries(-fsanitize=address)
endif()

option(WITH_TSAN_OPTION "Build sentry-native with thread sanitizer" OFF)
if(WITH_TSAN_OPTION)
	add_compile_options(-g -fsanitize=thread -fno-omit-frame-pointer)
	link_libraries(-fsanitize=thread)
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

function(sentry_install)
	if(SENTRY_ENABLE_INSTALL)
		install(${ARGN})
	endif()
endfunction()

# helper function to add sources to existing TARGET prepended with ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}
function(sentry_target_sources_cwd TARGET)
	cmake_parse_arguments(STSC "" "SUBDIR" "" ${ARGN})
	foreach(src ${STSC_UNPARSED_ARGUMENTS})
		if(IS_ABSOLUTE "${src}")
			target_sources(${TARGET} PRIVATE ${src})
		else()
			target_sources(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${STSC_SUBDIR}/${src}")
		endif()
	endforeach()
endfunction()

# ===== sentry library =====

add_library(sentry ${SENTRY_LIBRARY_TYPE} "${PROJECT_SOURCE_DIR}/vendor/mpack.c")
target_sources(sentry PRIVATE "${PROJECT_SOURCE_DIR}/include/sentry.h")
add_library(sentry::sentry ALIAS sentry)
add_subdirectory(src)

sentry_platform_modify_target(sentry)

if (NOT SENTRY_SDK_NAME STREQUAL "")
	target_compile_definitions(sentry PRIVATE SENTRY_SDK_NAME="${SENTRY_SDK_NAME}")
endif()

set_target_properties(sentry PROPERTIES PUBLIC_HEADER "include/sentry.h")

if(DEFINED SENTRY_FOLDER)
	set_target_properties(sentry PROPERTIES FOLDER ${SENTRY_FOLDER})
endif()

# check size type
include(CheckTypeSize)
check_type_size("long" CMAKE_SIZEOF_LONG)

if(SENTRY_BUILD_SHARED_LIBS)
	target_compile_definitions(sentry PRIVATE SENTRY_BUILD_SHARED)
else()
	target_compile_definitions(sentry PUBLIC SENTRY_BUILD_STATIC)
endif()
target_compile_definitions(sentry PRIVATE SIZEOF_LONG=${CMAKE_SIZEOF_LONG})

if(SENTRY_TRANSPORT_CURL)
	if(NOT TARGET CURL::libcurl) # Some other lib might bring libcurl already
		find_package(CURL REQUIRED COMPONENTS AsynchDNS)
	endif()

	target_link_libraries(sentry PRIVATE CURL::libcurl)
endif()

if(SENTRY_TRANSPORT_COMPRESSION)
	if(NOT TARGET ZLIB::ZLIB)
		find_package(ZLIB REQUIRED)
	endif()

	if(SENTRY_BACKEND_CRASHPAD)
		set(CRASHPAD_ZLIB_SYSTEM ON CACHE BOOL "Force CRASHPAD_ZLIB_SYSTEM when enabling transport compression" FORCE)
	endif()

	target_link_libraries(sentry PRIVATE ZLIB::ZLIB)
	target_compile_definitions(sentry PRIVATE SENTRY_TRANSPORT_COMPRESSION)
endif()

set_property(TARGET sentry PROPERTY C_VISIBILITY_PRESET hidden)

target_include_directories(sentry
	PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

include(cmake/utils.cmake)
if (WIN32 AND SENTRY_BUILD_SHARED_LIBS)
	sentry_add_version_resource(sentry "Client Library")
endif()

if(SENTRY_TRANSPORT_WINHTTP)
	list(APPEND _SENTRY_PLATFORM_LIBS "winhttp")
endif()

# apply platform libraries to sentry library
target_link_libraries(sentry PRIVATE ${_SENTRY_PLATFORM_LIBS})

# suppress some errors and warnings for MinGW target
if(MINGW)
	target_compile_options(sentry PRIVATE
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-format
		-Wno-incompatible-pointer-types
		-Wno-incompatible-function-pointer-types
	)
endif()

if(SENTRY_WITH_LIBUNWINDSTACK)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/libunwindstack-ndk/include>")
	add_subdirectory("${PROJECT_SOURCE_DIR}/external/libunwindstack-ndk/cmake")
	target_link_libraries(sentry PRIVATE unwindstack)
	if(NOT SENTRY_BUILD_SHARED_LIBS)
		sentry_install(TARGETS unwindstack EXPORT sentry
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
endif()

if(SENTRY_BACKEND_CRASHPAD)
	if(SENTRY_BUILD_SHARED_LIBS)
		set(CRASHPAD_ENABLE_INSTALL OFF CACHE BOOL "Enable crashpad installation" FORCE)
	else()
		set(CRASHPAD_ENABLE_INSTALL ON CACHE BOOL "Enable crashpad installation" FORCE)
	endif()
	add_subdirectory(external/crashpad crashpad_build)

	if(WIN32)
		add_dependencies(sentry crashpad::wer)
	endif()

	# set static runtime if enabled
	if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
		set_property(TARGET crashpad_client PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_compat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_getopt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_handler PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_handler_lib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_minidump PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_snapshot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_tools PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_util PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_wer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET crashpad_zlib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set_property(TARGET mini_chromium PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()

	if(DEFINED SENTRY_FOLDER)
		set_target_properties(crashpad_client PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_compat PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_getopt PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_handler PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_handler_lib PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_minidump PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_snapshot PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_tools PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_util PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_zlib PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(mini_chromium PROPERTIES FOLDER ${SENTRY_FOLDER})
		set_target_properties(crashpad_wer PROPERTIES FOLDER ${SENTRY_FOLDER})
	endif()

	target_link_libraries(sentry PRIVATE
		$<BUILD_INTERFACE:crashpad::client>
		$<INSTALL_INTERFACE:sentry_crashpad::client>
	)
	install(EXPORT crashpad_export NAMESPACE sentry_crashpad:: FILE sentry_crashpad-targets.cmake
		DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
	)

	sentry_platform_modify_crashpad_target(crashpad_handler)

	add_dependencies(sentry crashpad::handler)
elseif(SENTRY_BACKEND_BREAKPAD)
	option(SENTRY_BREAKPAD_SYSTEM "Use system breakpad" OFF)
	if(SENTRY_BREAKPAD_SYSTEM)
		target_compile_definitions(sentry PRIVATE SENTRY_BREAKPAD_SYSTEM)
		# system breakpad is using pkg-config, see `external/breakpad/breakpad-client.pc.in`
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(BREAKPAD REQUIRED IMPORTED_TARGET breakpad-client)
		if(SENTRY_BUILD_SHARED_LIBS)
			target_link_libraries(sentry PRIVATE PkgConfig::BREAKPAD)
		else()
			target_link_libraries(sentry PUBLIC PkgConfig::BREAKPAD)
		endif()
	else()
		add_subdirectory(external)
		target_include_directories(sentry PRIVATE
			"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/breakpad/src>"
		)
		target_link_libraries(sentry PRIVATE
			breakpad_client
		)

		if(DEFINED SENTRY_FOLDER)
			set_target_properties(breakpad_client PROPERTIES FOLDER ${SENTRY_FOLDER})
		endif()

		if(NOT SENTRY_BUILD_SHARED_LIBS)
			sentry_install(TARGETS breakpad_client EXPORT sentry
				LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			)
		endif()
	endif()
elseif(SENTRY_BACKEND_INPROC)
	target_compile_definitions(sentry PRIVATE SENTRY_WITH_INPROC_BACKEND)
endif()

option(SENTRY_INTEGRATION_QT "Build Qt integration")
if(SENTRY_INTEGRATION_QT)
	if(QT_DEFAULT_MAJOR_VERSION)
		# Let user choose major version
		set(Qt_VERSION_MAJOR ${QT_DEFAULT_MAJOR_VERSION})
	else()
		# Find best match, prioritizing Qt 6 if available
		find_package(Qt NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
	endif()
	find_package(Qt${Qt_VERSION_MAJOR} COMPONENTS Core REQUIRED)
	message(STATUS "Found Qt: ${Qt${Qt_VERSION_MAJOR}_DIR} "
		"(found version \"${Qt${Qt_VERSION_MAJOR}_VERSION}\")")
	target_link_libraries(sentry PRIVATE Qt${Qt_VERSION_MAJOR}::Core)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(sentry-config.cmake.in sentry-config.cmake
	INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

# generate package version file
# We would have liked to use `SameMinorVersion`, but that is only supported on
# CMake >= 3.11.
write_basic_package_version_file(sentry-config-version.cmake
	VERSION ${SENTRY_VERSION}
	COMPATIBILITY SameMajorVersion)

sentry_install(TARGETS sentry EXPORT sentry
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
sentry_install(EXPORT sentry NAMESPACE sentry:: FILE sentry-targets.cmake
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
sentry_install(
	FILES
		"${PROJECT_BINARY_DIR}/sentry-config.cmake"
		"${PROJECT_BINARY_DIR}/sentry-config-version.cmake"
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

sentry_platform_install()

# ===== tests =====

if(SENTRY_BUILD_TESTS)
	add_subdirectory(tests/unit)
endif()

# ===== example, also used as integration test =====

if(SENTRY_BUILD_EXAMPLES)
	add_executable(sentry_example examples/example.c)

	sentry_platform_modify_example_target(sentry_example)

	target_link_libraries(sentry_example PRIVATE sentry)

	if(MSVC)
		target_compile_options(sentry_example PRIVATE $<BUILD_INTERFACE:/wd5105 /wd4717>)

		# to test handling SEH by-passing exceptions we need to enable the control flow guard
		target_compile_options(sentry_example PRIVATE $<BUILD_INTERFACE:/guard:cf>)
	else()
		# Disable all optimizations for the `sentry_example` in gcc/clang. This allows us to keep crash triggers simple.
		# The effects besides reproducible code-gen across compiler versions, will be negligible for build- and runtime.
		target_compile_options(sentry_example PRIVATE $<BUILD_INTERFACE:-O0>)
	endif()

	# set static runtime if enabled
	if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
		set_property(TARGET sentry_example PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()

	if(DEFINED SENTRY_FOLDER)
		set_target_properties(sentry_example PROPERTIES FOLDER ${SENTRY_FOLDER})
	endif()

	add_custom_command(TARGET sentry_example POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/minidump.dmp" "$<TARGET_FILE_DIR:sentry_example>/minidump.dmp")

	add_test(NAME sentry_example COMMAND sentry_example)
endif()

# Limit the exported symbols when sentry is built as a shared library to those with a "sentry_" prefix:
# - we do this at the end of the file as to not affect subdirectories reading target_link_libraries from the parent.
# - we do this as PRIVATE since our version script does not make sense in any other project that adds us.
#
# Used linker parameters:
# `--build-id`: To have a build-id in the ELF object.
# `--version-script`: version script either hides "foreign" symbols or defers them as unknown ("U") to system libraries.
# FIXME: cmake 3.13 introduced target_link_options (blocked by Android)
if(SENTRY_BUILD_SHARED_LIBS)
	target_link_libraries(sentry PRIVATE
			"$<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Android>>:-Wl,--build-id=sha1,--version-script=${PROJECT_SOURCE_DIR}/src/exports.map>")

	# Support 16KB page sizes
	target_link_libraries(sentry PRIVATE
	    "$<$<PLATFORM_ID:Android>:-Wl,-z,max-page-size=16384>"
	)
endif()
