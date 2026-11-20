cmake_minimum_required(VERSION 3.10)
project(sentry_pkgconfig C)

# Options that match the main sentry-native build
option(SENTRY_BUILD_FORCE32 "Force a 32bit compile on a 64bit host" OFF)
option(WITH_ASAN_OPTION "Build with address sanitizer" OFF)
option(WITH_TSAN_OPTION "Build with thread sanitizer" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SENTRY REQUIRED IMPORTED_TARGET sentry)

pkg_get_variable(SENTRY_BINDIR sentry bindir)
find_program(SENTRY_HANDLER_PATH sentry-crash
	PATHS "${SENTRY_BINDIR}"
	NO_DEFAULT_PATH ONLY_CMAKE_FIND_ROOT_PATH)

add_executable(sentry_test_pkgconfig main.c)
target_link_libraries(sentry_test_pkgconfig PRIVATE PkgConfig::SENTRY)
target_compile_definitions(sentry_test_pkgconfig PRIVATE
	SENTRY_HANDLER_PATH=\"${SENTRY_HANDLER_PATH}\")

if(NOT MSVC)
	if(SENTRY_BUILD_FORCE32)
		target_compile_options(sentry_test_pkgconfig PRIVATE -m32)
		target_link_libraries(sentry_test_pkgconfig PRIVATE -m32)
	endif()

	if(WITH_ASAN_OPTION)
		target_compile_options(sentry_test_pkgconfig PRIVATE -fsanitize=address)
		target_link_libraries(sentry_test_pkgconfig PRIVATE -fsanitize=address)
	endif()

	if(WITH_TSAN_OPTION)
		target_compile_options(sentry_test_pkgconfig PRIVATE -fsanitize=thread)
		target_link_libraries(sentry_test_pkgconfig PRIVATE -fsanitize=thread)
	endif()
endif()
