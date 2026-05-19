cmake_minimum_required(VERSION 3.10)
project(sentry-native-ndk-sample LANGUAGES C CXX)

set(BUILD_SHARED_LIBS ON)
set(SENTRY_BUILD_SHARED_LIBS ON)

add_library(ndk-sample SHARED src/main/cpp/ndk-sample.cpp)

# Use sentry-native from the NDK library's Prefab package
find_package(sentry-native-ndk REQUIRED CONFIG)

# Android logging library
find_library(LOG_LIB log)

target_link_libraries(ndk-sample PRIVATE
	${LOG_LIB}
	sentry-native-ndk::sentry
)
# Support 16KB page sizes
target_link_options(ndk-sample PRIVATE "-Wl,-z,max-page-size=16384")
