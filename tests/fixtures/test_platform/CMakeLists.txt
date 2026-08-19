cmake_minimum_required(VERSION 3.10)
project(sentry_test_platform LANGUAGES C)

target_compile_definitions(sentry PRIVATE
    SENTRY_SDK_NAME="sentry.native.test")

add_executable(sentry_test_platform test_platform.c)
target_include_directories(sentry_test_platform PRIVATE ${SENTRY_SOURCE_DIR}/src)
target_link_libraries(sentry_test_platform PRIVATE sentry)
