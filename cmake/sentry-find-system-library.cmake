# Locates a system library the way CMake's own find modules do: `pkg-config` supplies search hints
# when it happens to be installed, and `find_library()`/`find_path()` do the actual lookup either way.
# See `FindLibinput`, `FindFontconfig` and the other bundled modules that follow the same shape.
#
# The only places sentry-native consults `pkg-config` are the optional `SENTRY_LIBUNWIND_SYSTEM` and
# `SENTRY_BREAKPAD_SYSTEM` code paths, and only on Linux. Requesting it with
# `find_package(PkgConfig REQUIRED)` nevertheless turned the tool into a hard build requirement for
# anyone packaging sentry-native, which is why package managers end up declaring it on every platform,
# including those where it is never invoked.
#
# Defines the imported target `TARGET` on success, and fails through
# `find_package_handle_standard_args()` with the usual "could not find" diagnostic otherwise.
#
#	sentry_find_system_library(<Name>
#		TARGET <target>
#		PKG_CONFIG_MODULE <module>
#		LIBRARY_NAMES <name>...
#		[HEADER_NAMES <header>...]
#		[HEADER_PATH_SUFFIXES <suffix>...])
function(sentry_find_system_library NAME)
	cmake_parse_arguments(SFSL "" "TARGET;PKG_CONFIG_MODULE"
		"LIBRARY_NAMES;HEADER_NAMES;HEADER_PATH_SUFFIXES" ${ARGN})

	if(TARGET "${SFSL_TARGET}")
		return()
	endif()

	# Hints only. A missing tool or a missing `.pc` file just leaves the hints empty. The guard is
	# needed because `pkg_check_modules()` is defined by `FindPkgConfig` itself, so it does not exist
	# when that module was never loaded.
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(PC_${NAME} QUIET "${SFSL_PKG_CONFIG_MODULE}")
	endif()

	set(libraries "")
	set(required_vars "")
	foreach(library_name IN LISTS SFSL_LIBRARY_NAMES)
		string(MAKE_C_IDENTIFIER "${NAME}_${library_name}_LIBRARY" cache_var)
		find_library("${cache_var}" NAMES "${library_name}" HINTS ${PC_${NAME}_LIBRARY_DIRS})
		mark_as_advanced("${cache_var}")
		list(APPEND libraries "${${cache_var}}")
		list(APPEND required_vars "${cache_var}")
	endforeach()

	if(SFSL_HEADER_NAMES)
		find_path(${NAME}_INCLUDE_DIR
			NAMES ${SFSL_HEADER_NAMES}
			HINTS ${PC_${NAME}_INCLUDE_DIRS}
			PATH_SUFFIXES ${SFSL_HEADER_PATH_SUFFIXES})
		mark_as_advanced(${NAME}_INCLUDE_DIR)
		list(APPEND required_vars ${NAME}_INCLUDE_DIR)
	endif()

	# These lookups only run once the user has opted into a system library, so a miss is fatal.
	# `find_package_handle_standard_args()` reads this to pick the failure mode and the message.
	set(${NAME}_FIND_REQUIRED TRUE)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(${NAME} REQUIRED_VARS ${required_vars})

	add_library("${SFSL_TARGET}" INTERFACE IMPORTED)
	set_target_properties("${SFSL_TARGET}" PROPERTIES INTERFACE_LINK_LIBRARIES "${libraries}")
	if(${NAME}_INCLUDE_DIR)
		set_target_properties("${SFSL_TARGET}" PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${NAME}_INCLUDE_DIR}")
	endif()
	if(PC_${NAME}_CFLAGS_OTHER)
		set_target_properties("${SFSL_TARGET}" PROPERTIES
			INTERFACE_COMPILE_OPTIONS "${PC_${NAME}_CFLAGS_OTHER}")
	endif()
endfunction()

# The lookups below are shared between the build itself and the installed `sentry-config.cmake`, which
# has to recreate the same imported targets for consumers of a static sentry-native. Keeping them here
# means the two cannot drift.

function(sentry_find_libunwind)
	sentry_find_system_library(SentryLibunwind
		TARGET sentry::libunwind
		PKG_CONFIG_MODULE libunwind
		LIBRARY_NAMES unwind
		HEADER_NAMES libunwind.h)
endfunction()

function(sentry_find_libunwind_ptrace)
	# `libunwind-ptrace.pc` declares `Requires: libunwind-generic libunwind`; the generic half has to
	# be named here, and callers link `sentry::libunwind` alongside for the other.
	sentry_find_system_library(SentryLibunwindPtrace
		TARGET sentry::libunwind-ptrace
		PKG_CONFIG_MODULE libunwind-ptrace
		LIBRARY_NAMES unwind-ptrace unwind-generic
		HEADER_NAMES libunwind-ptrace.h)
endfunction()

function(sentry_find_breakpad_client)
	# `breakpad-client.pc` exposes the headers below `${includedir}/breakpad`, matching the
	# `client/<os>/...` includes in `src/backends/sentry_backend_breakpad.cpp`.
	sentry_find_system_library(SentryBreakpadClient
		TARGET sentry::breakpad-client
		PKG_CONFIG_MODULE breakpad-client
		LIBRARY_NAMES breakpad_client
		HEADER_NAMES google_breakpad/common/breakpad_types.h
		HEADER_PATH_SUFFIXES breakpad)
endfunction()
