# Generates a version resource file from the `sentry.rc.in` template for the `TGT` argument and adds it as a source.
function(sentry_add_version_resource TGT FILE_DESCRIPTION)
	# generate a multi-config aware resource output-path from the target name
	set(RESOURCE_BASENAME "${TGT}.rc")
	set(RESOURCE_PATH_TMP "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_BASENAME}.in")
	set(RESOURCE_PATH "${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:$<CONFIG>>,$<CONFIG>/,>${RESOURCE_BASENAME}")

	# Produce the resource file with configure-time replacements
	configure_file("${SENTRY_SOURCE_DIR}/sentry.rc.in" "${RESOURCE_PATH_TMP}" @ONLY)

	# Replace the `ORIGINAL_FILENAME` at generate-time using the generator expression `TARGET_FILE_NAME`
	file(GENERATE OUTPUT ${RESOURCE_PATH} INPUT ${RESOURCE_PATH_TMP})

	# Finally add the generated resource file to the target sources
	target_sources("${TGT}" PRIVATE "${RESOURCE_PATH}")
endfunction()

function(sentry_get_property NAME)
	get_target_property(prop sentry "${NAME}")
	if(NOT prop)
		set(prop)
	endif()
	set("SENTRY_${NAME}" "${prop}" PARENT_SCOPE)
endfunction()

function(sentry_find_atomic_library OUT_VAR)
	include(CheckCSourceCompiles)
	include(CMakePushCheckState)
	cmake_push_check_state(RESET)
	set(ATOMIC_U64_SOURCE "
		#include <stdint.h>
		uint64_t value;
		int main(void) {
			return (int)__atomic_load_n(&value, __ATOMIC_SEQ_CST);
		}")
	check_c_source_compiles("${ATOMIC_U64_SOURCE}" SENTRY_HAVE_ATOMIC_U64)
	if(SENTRY_HAVE_ATOMIC_U64)
		set(ATOMIC_LIBRARY)
	else()
		set(CMAKE_REQUIRED_LIBRARIES atomic)
		check_c_source_compiles("${ATOMIC_U64_SOURCE}" SENTRY_HAVE_ATOMIC_U64_WITH_LIB)
		if(NOT SENTRY_HAVE_ATOMIC_U64_WITH_LIB)
			message(FATAL_ERROR "64-bit atomic operations are not supported")
		endif()
		set(ATOMIC_LIBRARY atomic)
	endif()
	cmake_pop_check_state()
	set(${OUT_VAR} "${ATOMIC_LIBRARY}" PARENT_SCOPE)
endfunction()
