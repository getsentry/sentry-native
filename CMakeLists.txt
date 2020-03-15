cmake_minimum_required (VERSION 3.10)
project(Sentry-Native LANGUAGES C CXX ASM)

include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/sentry")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LINUX TRUE)
endif()

if(APPLE OR WIN32)
	set(SENTRY_DEFAULT_BACKEND "crashpad")
elseif(LINUX)
	set(SENTRY_DEFAULT_BACKEND "breakpad")
else()
	set(SENTRY_DEFAULT_BACKEND "inproc")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)

option(SENTRY_BUILD_TESTS "Build sentry-native tests" ON)
option(SENTRY_BUILD_EXAMPLES "Build sentry-native example(s)" ON)

if(SENTRY_BUILD_TESTS OR SENTRY_BUILD_EXAMPLES)
	enable_testing()
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
	set(SENTRY_MAIN_PROJECT ON)
endif()

option(SENTRY_ENABLE_INSTALL "Enable sentry installation" "${SENTRY_MAIN_PROJECT}")

function(sentry_install)
	if(SENTRY_ENABLE_INSTALL)
		install(${ARGN})
	endif()
endfunction()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(SENTRY_BACKEND ${SENTRY_DEFAULT_BACKEND} CACHE STRING
  "The sentry backend responsible for reporting crashes, can be either 'none', 'inproc', 'breakpad' or 'crashpad'.")

if(SENTRY_BACKEND STREQUAL "crashpad")
	set(BACKEND_CRASHPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "inproc")
	set(BACKEND_INPROC TRUE)
elseif(SENTRY_BACKEND STREQUAL "breakpad")
	set(BACKEND_BREAKPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "none")
	set(BACKEND_NONE TRUE)
else()
	message(FATAL_ERROR "SENTRY_BACKEND must be one of 'crashpad', 'inproc', 'breakpad' or 'none'")
endif()

if(BACKEND_CRASHPAD AND NOT (APPLE OR WIN32))
	message(FATAL_ERROR "The Crashpad backend is currently only supported on macOS and Windows")
endif()
if(BACKEND_BREAKPAD AND NOT (LINUX OR ANDROID))
	message(FATAL_ERROR "The Breakpad backend is currently only supported on Linux and Android")
endif()
if(BACKEND_INPROC AND WIN32)
	message(FATAL_ERROR "The in-process backend is not supported on Windows")
endif()

if(ANDROID)
	set(WITH_LIBUNWINDSTACK TRUE)
elseif(NOT WIN32)
	set(WITH_LIBBACKTRACE TRUE)
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

option(SENTRY_CURL_SUPPORT "Builds sentry-native with curl support" ON)

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
endif()

if(BUILD_SHARED_LIBS)
	target_compile_definitions(sentry PRIVATE SENTRY_BUILD_SHARED)
else()
	target_compile_definitions(sentry PUBLIC SENTRY_BUILD_STATIC)
endif()

if(SENTRY_CURL_SUPPORT)
	find_package(CURL REQUIRED)
	target_compile_definitions(sentry PUBLIC SENTRY_WITH_LIBCURL_TRANSPORT)
	target_include_directories(sentry PRIVATE ${CURL_INCLUDE_DIR})
	# The exported sentry target must not contain any path of the build machine, therefore use generator expressions
	# FIXME: cmake 3.12 introduced the target CURL::libcurl
	string(REPLACE ";" "$<SEMICOLON>" GENEX_CURL_LIBRARIES "${CURL_LIBRARIES}")
	string(REPLACE ";" "$<SEMICOLON>" GENEX_CURL_COMPILE_DEFINITIONS "${CURL_COMPILE_DEFINITIONS}")
	target_link_libraries(sentry PRIVATE $<BUILD_INTERFACE:${GENEX_CURL_LIBRARIES}>)
	target_compile_definitions(sentry PRIVATE $<BUILD_INTERFACE:${GENEX_CURL_COMPILE_DEFINITIONS}>)
endif()

if(WIN32)
	target_compile_definitions(sentry
		PRIVATE WINVER=0x0603 NTDDI_VERSION=0x06030000
		PUBLIC SENTRY_WITH_WINHTTP_TRANSPORT
	)
	target_link_libraries(sentry PRIVATE winhttp)
endif()

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration -Werror=incompatible-function-pointer-types -Wall -fvisibility=hidden")
set_property(TARGET sentry PROPERTY C_VISIBILITY_PRESET hidden)
if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CMAKE_ASM_MASM_FLAGS "${CMAKE_ASM_MASM_FLAGS} /safeseh")
	endif()
else()
	target_compile_options(sentry PUBLIC $<BUILD_INTERFACE:-Wall>)
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
endif()

if(WITH_LIBUNWINDSTACK)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/libunwindstack-ndk/include>")
	add_subdirectory("${PROJECT_SOURCE_DIR}/external/libunwindstack-ndk/cmake")
	target_link_libraries(sentry PRIVATE unwindstack)
	if(NOT BUILD_SHARED_LIBS)
		sentry_install(TARGETS unwindstack EXPORT sentry
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)
	endif()
endif()

if(MSVC)
	target_link_libraries(sentry PRIVATE dbghelp pathcch)
elseif(MINGW)
	target_compile_definitions(sentry PRIVATE 
		__LP64__ #force 64bits flag to remove some -Wincompatible-pointer-types errors 
	)
	target_link_libraries(sentry PRIVATE dbghelp shlwapi) #shlwapi as replacement for newer and unimplemented pathcch
	target_compile_options(sentry PRIVATE -Wno-unused-variable)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_definitions(sentry PRIVATE 
			__USE_MINGW_ANSI_STDIO=1 #prevent a bunch of -Wformat= errors
		)
	endif()
endif()

if(BACKEND_CRASHPAD)
	add_subdirectory(external/crashpad EXCLUDE_FROM_ALL)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/crashpad>"
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/crashpad/third_party/mini_chromium/mini_chromium>"
	)
	# FIXME: this should be applied to mini_chromium (or at least to crashpad_client/crashpad_handler)
	target_compile_features(sentry PUBLIC cxx_std_11)
	target_link_libraries(sentry PRIVATE crashpad_client crashpad_util)
	if(NOT BUILD_SHARED_LIBS)
		sentry_install(TARGETS crashpad_client crashpad_util crashpad_compat getopt mini_chromium zlib EXPORT sentry
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)
	endif()
	sentry_install(TARGETS crashpad_handler
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
	add_dependencies(sentry crashpad_handler)
elseif(BACKEND_BREAKPAD)
	add_subdirectory(external)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/breakpad/src>"
	)
	target_link_libraries(sentry PRIVATE
		breakpad_client
	)
	if(NOT BUILD_SHARED_LIBS)
		sentry_install(TARGETS breakpad_client EXPORT sentry
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)
	endif()

elseif(BACKEND_INPROC)
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
