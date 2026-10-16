cmake_minimum_required(VERSION 3.10)
project(sentry_pkgconfig C)

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
