cmake_minimum_required(VERSION 3.10)

# enables support for CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
endif()

project(sentry_crash_reporter LANGUAGES C)

add_executable(sentry_crash_reporter crash_reporter.c)
target_link_libraries(sentry_crash_reporter PRIVATE sentry)
