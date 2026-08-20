# Locates a system library, preferring `pkg-config` metadata when both the tool and the requested module are
# available, and falling back to plain `find_library()`/`find_path()` when they are not.
#
# `pkg-config` is only ever consulted for the optional `SENTRY_LIBUNWIND_SYSTEM` and `SENTRY_BREAKPAD_SYSTEM` code
# paths, and only on Linux. Requesting it with `find_package(PkgConfig REQUIRED)` nevertheless turned the tool into a
# hard build requirement for anyone packaging sentry-native, which is why package managers end up declaring it on
# every platform, including those where it is never invoked.
#
# On success the imported target `TGT` is defined regardless of which of the two lookups provided it, so neither the
# call sites nor the installed `sentry-config.cmake` have to branch on the outcome.
#
#	sentry_find_system_library(<TGT>
#		PKG_CONFIG_MODULE <module>
#		LIBRARY_NAMES <name>...
#		[HEADER_NAMES <header>...]
#		[HEADER_PATH_SUFFIXES <suffix>...])
function(sentry_find_system_library TGT)
	cmake_parse_arguments(SFSL "" "PKG_CONFIG_MODULE" "LIBRARY_NAMES;HEADER_NAMES;HEADER_PATH_SUFFIXES" ${ARGN})

	if(TARGET "${TGT}")
		return()
	endif()

	string(MAKE_C_IDENTIFIER "${TGT}" prefix)
	string(TOUPPER "${prefix}" prefix)

	# `pkg-config` resolves transitive `Requires:` and `Libs.private:` entries for us, so prefer it when available.
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules("${prefix}" QUIET IMPORTED_TARGET "${SFSL_PKG_CONFIG_MODULE}")
	endif()

	if(TARGET "PkgConfig::${prefix}")
		add_library("${TGT}" INTERFACE IMPORTED)
		set_target_properties("${TGT}" PROPERTIES INTERFACE_LINK_LIBRARIES "PkgConfig::${prefix}")
		return()
	endif()

	# No usable `pkg-config` module, so resolve the library and its headers ourselves.
	set(libraries "")
	set(missing "")
	foreach(name IN LISTS SFSL_LIBRARY_NAMES)
		string(MAKE_C_IDENTIFIER "SENTRY_${prefix}_${name}_LIBRARY" cache_var)
		find_library("${cache_var}" NAMES "${name}")
		mark_as_advanced("${cache_var}")
		if(${cache_var})
			list(APPEND libraries "${${cache_var}}")
		else()
			list(APPEND missing "lib${name}")
		endif()
	endforeach()

	set(include_dir "")
	if(SFSL_HEADER_NAMES)
		string(MAKE_C_IDENTIFIER "SENTRY_${prefix}_INCLUDE_DIR" cache_var)
		if(SFSL_HEADER_PATH_SUFFIXES)
			find_path("${cache_var}" NAMES ${SFSL_HEADER_NAMES} PATH_SUFFIXES ${SFSL_HEADER_PATH_SUFFIXES})
		else()
			find_path("${cache_var}" NAMES ${SFSL_HEADER_NAMES})
		endif()
		mark_as_advanced("${cache_var}")
		if(${cache_var})
			set(include_dir "${${cache_var}}")
		else()
			list(GET SFSL_HEADER_NAMES 0 header)
			list(APPEND missing "${header}")
		endif()
	endif()

	if(missing)
		string(REPLACE ";" ", " missing "${missing}")
		message(FATAL_ERROR
			"Could not find the system dependency `${SFSL_PKG_CONFIG_MODULE}` needed for `${TGT}`.\n"
			"Missing: ${missing}.\n"
			"Install the matching development package, point CMake at it via `CMAKE_PREFIX_PATH`, or install "
			"`pkg-config`/`pkgconf` so that `${SFSL_PKG_CONFIG_MODULE}.pc` can be used instead.")
	endif()

	add_library("${TGT}" INTERFACE IMPORTED)
	set_target_properties("${TGT}" PROPERTIES INTERFACE_LINK_LIBRARIES "${libraries}")
	if(include_dir)
		set_target_properties("${TGT}" PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_dir}")
	endif()
endfunction()

# The lookups below are shared between the build itself and the installed `sentry-config.cmake`, which has to recreate
# the same imported targets for consumers of a static sentry-native. Keeping them here means the two cannot drift.

function(sentry_find_libunwind)
	sentry_find_system_library(sentry::libunwind
		PKG_CONFIG_MODULE libunwind
		LIBRARY_NAMES unwind
		HEADER_NAMES libunwind.h)
endfunction()

function(sentry_find_libunwind_ptrace)
	# `libunwind-ptrace.pc` pulls in `libunwind-generic`, so the fallback has to link it explicitly.
	sentry_find_system_library(sentry::libunwind-ptrace
		PKG_CONFIG_MODULE libunwind-ptrace
		LIBRARY_NAMES unwind-ptrace unwind-generic
		HEADER_NAMES libunwind-ptrace.h)
endfunction()

function(sentry_find_breakpad_client)
	# `breakpad-client.pc` exposes the headers below `${includedir}/breakpad`, matching the `client/<os>/...`
	# includes in `src/backends/sentry_backend_breakpad.cpp`.
	sentry_find_system_library(sentry::breakpad-client
		PKG_CONFIG_MODULE breakpad-client
		LIBRARY_NAMES breakpad_client
		HEADER_NAMES google_breakpad/common/breakpad_types.h
		HEADER_PATH_SUFFIXES breakpad)
endfunction()
