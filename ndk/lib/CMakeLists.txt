cmake_minimum_required(VERSION 3.10)
project(Sentry-Android LANGUAGES C CXX)

if(POLICY CMP0079)
	cmake_policy(SET CMP0079 NEW)
endif()

# Add sentry-android shared library
add_library(sentry-android SHARED src/main/jni/sentry.c)

# Conditionally add test library if ENABLE_TESTS is set
if(ENABLE_TESTS)
	add_library(sentry-android-test SHARED src/androidTest/jni/ndk-test.cpp)
endif()

# make sure that we build it as a shared lib instead of a static lib
set(BUILD_SHARED_LIBS ON)
set(SENTRY_BUILD_SHARED_LIBS ON)
set(SENTRY_TRANSPORT "custom" CACHE STRING "" FORCE)

# Adding sentry-native project
add_subdirectory(${SENTRY_NATIVE_SRC} sentry_build)
target_sources(sentry PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main/jni/sentry_android_transport.c
)

if(SENTRY_BACKEND STREQUAL "native")
	find_package(curl REQUIRED CONFIG)
	# TODO: SENTRY_TRANSPORT=curl
	target_sources(sentry-crash PRIVATE
		${SENTRY_NATIVE_SRC}/src/transports/sentry_http_transport_curl.c
	)
	target_compile_definitions(sentry-android PRIVATE SENTRY_BACKEND_NATIVE)
	set_target_properties(sentry-crash PROPERTIES
		PREFIX "lib"
		OUTPUT_NAME "sentry-crash"
		SUFFIX ".so"
	)
	target_link_options(sentry-crash PRIVATE "-Wl,-z,max-page-size=16384")
	target_link_libraries(sentry-crash PRIVATE curl::curl_static)
	target_link_libraries(sentry-android PRIVATE dl)
endif()

# Android logging library
find_library(LOG_LIB log)

if(ENABLE_TESTS)
	target_link_libraries(sentry-android-test PRIVATE
		${LOG_LIB}
		$<BUILD_INTERFACE:sentry::sentry>
	)
endif()

# Link to sentry-native
target_link_libraries(sentry-android PRIVATE $<BUILD_INTERFACE:sentry::sentry>)

# Support 16KB page sizes
target_link_options(sentry-android PRIVATE "-Wl,-z,max-page-size=16384")
