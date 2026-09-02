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

function(sentry_check_cpp_language_features)
	include(CheckCXXSourceCompiles)
	unset(SENTRY_CPP_LANGUAGE_FEATURES CACHE)
	check_cxx_source_compiles("\
#if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)\n\
#error C++ exceptions are disabled\n\
#endif\n\
#if !defined(__cpp_rtti) && !defined(__GXX_RTTI) && !defined(_CPPRTTI)\n\
#error C++ RTTI is disabled\n\
#endif\n\
#include <exception>\n\
#include <typeinfo>\n\
int main() { try { throw 1; } catch (...) { return typeid(int) == typeid(int) ? 0 : 1; } }"
		SENTRY_CPP_LANGUAGE_FEATURES)
	if(NOT SENTRY_CPP_LANGUAGE_FEATURES)
		message(FATAL_ERROR
			"SENTRY_INTEGRATION_CPP requires C++ exceptions and RTTI")
	endif()
endfunction()
