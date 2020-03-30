cmake_minimum_required (VERSION 3.10)
project(Sentry-Native LANGUAGES C CXX ASM)

set(SENTRY_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(SENTRY_MAIN_PROJECT ON)
endif()

include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/sentry")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LINUX TRUE)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)

option(SENTRY_BUILD_TESTS "Build sentry-native tests" "${SENTRY_MAIN_PROJECT}")
option(SENTRY_BUILD_EXAMPLES "Build sentry-native example(s)" "${SENTRY_MAIN_PROJECT}")

if(WIN32)
	set(SENTRY_DEFAULT_TRANSPORT "winhttp")
elseif(APPLE OR LINUX)
	set(SENTRY_DEFAULT_TRANSPORT "curl")
else()
	set(SENTRY_DEFAULT_TRANSPORT "none")
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

if(APPLE OR WIN32)
	set(SENTRY_DEFAULT_BACKEND "crashpad")
elseif(LINUX)
	set(SENTRY_DEFAULT_BACKEND "breakpad")
else()
	set(SENTRY_DEFAULT_BACKEND "inproc")
endif()

set(SENTRY_BACKEND ${SENTRY_DEFAULT_BACKEND} CACHE STRING
  "The sentry backend responsible for reporting crashes, can be either 'none', 'inproc', 'breakpad' or 'crashpad'.")

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

if(SENTRY_BACKEND_CRASHPAD AND NOT (APPLE OR WIN32))
	message(FATAL_ERROR "The Crashpad backend is currently only supported on macOS and Windows")
endif()
if(SENTRY_BACKEND_BREAKPAD AND NOT LINUX)
	message(FATAL_ERROR "The Breakpad backend is currently only supported on Linux")
endif()
if(SENTRY_BACKEND_INPROC AND WIN32)
	message(FATAL_ERROR "The in-process backend is not supported on Windows")
endif()

message(STATUS "SENTRY_TRANSPORT=${SENTRY_TRANSPORT}")
message(STATUS "SENTRY_BACKEND=${SENTRY_BACKEND}")

if(ANDROID)
	set(SENTRY_WITH_LIBUNWINDSTACK TRUE)
elseif(NOT WIN32)
	set(SENTRY_WITH_LIBBACKTRACE TRUE)
endif()

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

#respect CMAKE_SYSTEM_VERSION
if(WIN32)
    if(${CMAKE_SYSTEM_VERSION} EQUAL 10)
        add_definitions(-D_WIN32_WINNT=0x0A00)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.3)
        add_definitions(-D_WIN32_WINNT=0x0603)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.2)
        add_definitions(-D_WIN32_WINNT=0x0602)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.1)
        add_definitions(-D_WIN32_WINNT=0x0601)
    elseif(${CMAKE_SYSTEM_VERSION} EQUAL 6.0)
        add_definitions(-D_WIN32_WINNT=0x0600)
    else()
        add_definitions(-D_WIN32_WINNT=0x0501)
    endif()
endif()

# ===== sentry library =====

add_library(sentry "${PROJECT_SOURCE_DIR}/vendor/mpack.c")
add_library(sentry::sentry ALIAS sentry)
add_subdirectory(src)

set_target_properties(sentry PROPERTIES PUBLIC_HEADER "include/sentry.h")

# https://gitlab.kitware.com/cmake/cmake/issues/18393
if(BUILD_SHARED_LIBS)
	if(APPLE)
		sentry_install(FILES "$<TARGET_FILE:sentry>.dSYM" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
	elseif(MSVC)
		sentry_install(FILES "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:$<TARGET_PDB_FILE:sentry>>"
			DESTINATION "${CMAKE_INSTALL_BINDIR}")
	endif()
elseif(LINUX)
	target_compile_options(sentry PRIVATE -fPIC)
endif()

if(BUILD_SHARED_LIBS)
	target_compile_definitions(sentry PRIVATE SENTRY_BUILD_SHARED)
else()
	target_compile_definitions(sentry PUBLIC SENTRY_BUILD_STATIC)
endif()

if(SENTRY_TRANSPORT_CURL)
	find_package(CURL REQUIRED)
	target_include_directories(sentry PRIVATE ${CURL_INCLUDE_DIR})
	# The exported sentry target must not contain any path of the build machine, therefore use generator expressions
	# FIXME: cmake 3.12 introduced the target CURL::libcurl
	string(REPLACE ";" "$<SEMICOLON>" GENEX_CURL_LIBRARIES "${CURL_LIBRARIES}")
	string(REPLACE ";" "$<SEMICOLON>" GENEX_CURL_COMPILE_DEFINITIONS "${CURL_COMPILE_DEFINITIONS}")
	target_link_libraries(sentry PRIVATE $<BUILD_INTERFACE:${GENEX_CURL_LIBRARIES}>)
	target_compile_definitions(sentry PRIVATE $<BUILD_INTERFACE:${GENEX_CURL_COMPILE_DEFINITIONS}>)
elseif(SENTRY_TRANSPORT_WINHTTP)
	target_link_libraries(sentry PRIVATE winhttp)
endif()

set_property(TARGET sentry PROPERTY C_VISIBILITY_PRESET hidden)
if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CMAKE_ASM_MASM_FLAGS "${CMAKE_ASM_MASM_FLAGS} /safeseh")
	endif()
else()
	target_compile_options(sentry PUBLIC $<BUILD_INTERFACE:-Wall -Wextra -Wpedantic>)
endif()

target_include_directories(sentry PUBLIC
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# Without this, dladdr can't find functions
# FIXME: cmake 3.13 introduced target_link_options
target_link_libraries(sentry INTERFACE
	"$<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Android>>:-Wl,--build-id=sha1>")

if(LINUX)
	target_link_libraries(sentry PRIVATE pthread dl)
elseif(MSVC)
	target_link_libraries(sentry PRIVATE dbghelp pathcch shlwapi)
elseif(MINGW)
	target_link_libraries(sentry PRIVATE dbghelp shlwapi) #unimplemented pathcch
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
	if(NOT BUILD_SHARED_LIBS)
		sentry_install(TARGETS unwindstack EXPORT sentry
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
endif()

if(SENTRY_BACKEND_CRASHPAD)
	add_subdirectory(external/crashpad EXCLUDE_FROM_ALL)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/crashpad>"
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/crashpad/third_party/mini_chromium/mini_chromium>"
	)
	# we include a crashpad header, which itself needs C++14 support
	target_compile_features(sentry PUBLIC cxx_std_14)
	target_link_libraries(sentry PRIVATE crashpad_client crashpad_util)
	if(NOT BUILD_SHARED_LIBS)
		sentry_install(TARGETS crashpad_client crashpad_util crashpad_compat getopt mini_chromium zlib EXPORT sentry
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
	sentry_install(TARGETS crashpad_handler EXPORT sentry
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
	add_dependencies(sentry crashpad_handler)
elseif(SENTRY_BACKEND_BREAKPAD)
	add_subdirectory(external)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/breakpad/src>"
	)
	target_link_libraries(sentry PRIVATE
		breakpad_client
	)
	if(NOT BUILD_SHARED_LIBS)
		sentry_install(TARGETS breakpad_client EXPORT sentry
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
elseif(SENTRY_BACKEND_INPROC)
	target_compile_definitions(sentry PRIVATE SENTRY_WITH_INPROC_BACKEND)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(sentry-config.cmake.in sentry-config.cmake
	INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

sentry_install(TARGETS sentry EXPORT sentry
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
sentry_install(EXPORT sentry NAMESPACE sentry:: FILE sentry-targets.cmake
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
sentry_install(FILES "${PROJECT_BINARY_DIR}/sentry-config.cmake"
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

# ===== tests =====

if(SENTRY_BUILD_TESTS)
	add_subdirectory(tests/unit)
endif()

# ===== example, also used as integration test =====

if(SENTRY_BUILD_EXAMPLES)
	add_executable(sentry_example examples/example.c)
	target_link_libraries(sentry_example sentry)
	add_test(NAME sentry_example COMMAND sentry_example)
endif()
