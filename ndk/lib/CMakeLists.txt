cmake_minimum_required(VERSION 3.10)
project(Sentry-Android LANGUAGES C CXX)

# Add sentry-android shared library
add_library(sentry-android SHARED src/main/jni/sentry.c)

# make sure that we build it as a shared lib instead of a static lib
set(BUILD_SHARED_LIBS ON)
set(SENTRY_BUILD_SHARED_LIBS ON)

# Adding sentry-native project
add_subdirectory(${SENTRY_NATIVE_SRC} sentry_build)

# Link to sentry-native
target_link_libraries(sentry-android PRIVATE $<BUILD_INTERFACE:sentry::sentry>)

# Support 16KB page sizes
target_link_options(sentry-android PRIVATE "-Wl,-z,max-page-size=16384")
