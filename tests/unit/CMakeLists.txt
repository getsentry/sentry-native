sentry_get_property(SOURCES)
sentry_get_property(COMPILE_DEFINITIONS)
sentry_get_property(INTERFACE_INCLUDE_DIRECTORIES)
sentry_get_property(INCLUDE_DIRECTORIES)
sentry_get_property(LINK_LIBRARIES)
sentry_get_property(INTERFACE_LINK_LIBRARIES)

list(FILTER SENTRY_SOURCES EXCLUDE REGEX "\\.rc$")

add_executable(sentry_test_unit
	${SENTRY_SOURCES}
	main.c
	sentry_testsupport.h
	test_batcher.c
	test_app_hang.c
	test_attachments.c
	test_basic.c
	test_cache.c
	test_client_report.c
	test_consent.c
	test_concurrency.c
	test_embedded_info.c
	test_envelopes.c
	test_failures.c
	test_feedback.c
	test_fuzzfailures.c
	test_http_transport.c
	test_info.c
	test_logger.c
	test_logs.c
	test_metrics.c
	test_modulefinder.c
	test_mpack.c
	test_native_backend.c
	test_options.c
	test_os.c
	test_path.c
	test_process.c
	test_ratelimiter.c
	test_retry.c
	test_ringbuffer.c
	test_sampling.c
	test_scope.c
	test_session.c
	test_slice.c
	test_string.c
	test_symbolizer.c
	test_sync.c
	test_tracing.c
	test_tus.c
	test_uninit.c
	test_unwinder.c
	test_utils.c
	test_uuid.c
	test_value.c
	test_writer.c
	tests.inc
)

# FIXME: cmake 3.13 introduced target_link_options
target_compile_definitions(sentry_test_unit PRIVATE ${SENTRY_COMPILE_DEFINITIONS})

# Add SENTRY_EMBED_INFO define to tests if embedding is enabled
if(SENTRY_EMBED_INFO)
	target_compile_definitions(sentry_test_unit PRIVATE SENTRY_EMBED_INFO=1)
endif()
target_include_directories(sentry_test_unit PRIVATE
	${SENTRY_INTERFACE_INCLUDE_DIRECTORIES}
	${SENTRY_INCLUDE_DIRECTORIES}
)
target_link_libraries(sentry_test_unit PRIVATE
	${SENTRY_LINK_LIBRARIES}
	${SENTRY_INTERFACE_LINK_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:rt>"
	"$<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Android>>:-Wl,-E,--build-id=sha1>"
)

if(MINGW)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_link_options(sentry_test_unit PRIVATE -Wl,-pdb=) # IMPORTANT ! LLD must generate pdb files
		target_compile_options(sentry_test_unit PRIVATE -gcodeview) # IMPORTANT ! Clang must generate pdb to CodeView format
	else()
		message(FATAL_ERROR "Tests will not pass on MinGW if your compiler cannot generate .pdb files ! Please use Clang instead")
	endif()
	target_compile_options(sentry_test_unit PRIVATE
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-format
		-Wno-incompatible-pointer-types
		-Wno-incompatible-function-pointer-types
	)
endif()

if(MSVC)
	target_compile_options(sentry_test_unit PRIVATE $<BUILD_INTERFACE:/wd5105>)
endif()

# set static runtime if enabled
if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
	set_property(TARGET sentry_test_unit PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_compile_definitions(sentry PRIVATE SIZEOF_LONG=${CMAKE_SIZEOF_LONG})

target_compile_definitions(sentry_test_unit PRIVATE SENTRY_UNITTEST)

# The backends discover their helper next to the current executable. Building
# only the unit-test target must also build that helper, including with the
# default output directories and multi-configuration generators.
if(TARGET crashpad_handler)
	set(sentry_test_helper crashpad_handler)
elseif(TARGET sentry-crash)
	set(sentry_test_helper sentry-crash)
endif()

if(sentry_test_helper)
	add_dependencies(sentry_test_unit ${sentry_test_helper})
	set_target_properties(sentry_test_unit PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:${sentry_test_helper}>")
	foreach(config ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
		string(TOUPPER "${config}" config_upper)
		set_target_properties(sentry_test_unit PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_${config_upper} "$<TARGET_FILE_DIR:${sentry_test_helper}>")
	endforeach()
endif()

option(SENTRY_CTEST_INDIVIDUAL "Register sentry unit tests as individual tests instead of a single binary" ON)

function(sentry_add_unit_test NAME)
	# Preserve the command prefixes normally supplied by add_test for a target.
	set(test_emulator "")
	set(use_emulator OFF)
	if(CMAKE_CROSSCOMPILING)
		set(use_emulator ON)
	elseif(POLICY CMP0158)
		cmake_policy(GET CMP0158 emulator_policy)
		if(NOT emulator_policy STREQUAL "NEW")
			set(use_emulator ON)
		endif()
	else()
		set(use_emulator ON)
	endif()
	if(use_emulator)
		set(test_emulator "$<TARGET_PROPERTY:sentry_test_unit,CROSSCOMPILING_EMULATOR>")
	endif()
	set(test_launcher "$<TARGET_PROPERTY:sentry_test_unit,TEST_LAUNCHER>")
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
		set(test_emulator "$<GENEX_EVAL:${test_emulator}>")
		set(test_launcher "$<GENEX_EVAL:${test_launcher}>")
	endif()
	add_test(NAME sentry/${NAME} COMMAND "${CMAKE_COMMAND}"
		"-DTEST_LAUNCHER=${test_launcher}"
		"-DTEST_EMULATOR=${test_emulator}"
		"-DTEST_EXECUTABLE=$<TARGET_FILE:sentry_test_unit>"
		"-DTEST_WORKING_ROOT=${CMAKE_CURRENT_BINARY_DIR}/ctest/$<CONFIG>"
		"-DTEST_NAME=${NAME}"
		"-DTEST_INDIVIDUAL=${SENTRY_CTEST_INDIVIDUAL}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake")
endfunction()

if(SENTRY_CTEST_INDIVIDUAL)
	file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/tests.inc unittests_file)
	foreach(line ${unittests_file})
		# extract "name" from "XX(name)"
		string(STRIP "${line}" line)
		string(REGEX MATCH "^XX\\(([^\\)]+)\\)$" _ "${line}")
		if(NOT _)
			message(WARNING "Failed to extract test name from line: ${line}")
		else()
			sentry_add_unit_test("${CMAKE_MATCH_1}")
		endif()
	endforeach()
else()
	sentry_add_unit_test(unit-tests)
endif()

add_executable(sentry_fuzz_json
	${SENTRY_SOURCES}
	fuzz.c
)
target_compile_definitions(sentry_fuzz_json PRIVATE ${SENTRY_COMPILE_DEFINITIONS})
target_include_directories(sentry_fuzz_json PRIVATE
	${SENTRY_INTERFACE_INCLUDE_DIRECTORIES}
	${SENTRY_INCLUDE_DIRECTORIES}
)
target_link_libraries(sentry_fuzz_json PRIVATE
	${SENTRY_LINK_LIBRARIES}
	${SENTRY_INTERFACE_LINK_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:rt>"
)

if(MSVC)
	target_compile_options(sentry_fuzz_json PRIVATE $<BUILD_INTERFACE:/wd5105>)
endif()

# set static runtime if enabled
if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
	set_property(TARGET sentry_fuzz_json PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
