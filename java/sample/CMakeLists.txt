cmake_minimum_required(VERSION 3.10)
project(Sentry-Sample LANGUAGES C CXX)

set(BUILD_SHARED_LIBS ON)
set(SENTRY_BUILD_SHARED_LIBS ON)

add_library(native-sample SHARED src/main/cpp/native-sample.cpp)

# Adding sentry-native project
add_subdirectory(${SENTRY_NATIVE_SRC} sentry_build)

# target_include_directories(native-sample PRIVATE ../../sentry-native/include)

find_library(LOG_LIB log)

target_link_libraries(native-sample PRIVATE
    ${LOG_LIB}
    $<BUILD_INTERFACE:sentry::sentry>
)
