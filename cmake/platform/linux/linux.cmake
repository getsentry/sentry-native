if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    macro(sentry_platform_setup)
        # Unused
    endmacro()

    macro(sentry_platform_init)
        set(LINUX TRUE)

        sentry_link_pthread()

        option(SENTRY_BUILD_FORCE32 "Force a 32bit compile on a 64bit host" OFF)
        if(SENTRY_BUILD_FORCE32)
            set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} -m32 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
            set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -m32 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
            set(CMAKE_ASM_FLAGS  "${CMAKE_ASM_FLAGS} -m32 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
            set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
        endif()

        set(SENTRY_DEFAULT_TRANSPORT "curl")

        set(SENTRY_DEFAULT_BACKEND "crashpad")

        foreach(lang ASM C CXX)
            string(REPLACE "-O2" "-O3" CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}")
        endforeach()
    endmacro()

    function(sentry_platform_modify_target target)
        target_sources(${target} PRIVATE
                "${PROJECT_SOURCE_DIR}/vendor/stb_sprintf.c"
                "${PROJECT_SOURCE_DIR}/vendor/stb_sprintf.h"
        )

        target_compile_options(${target} PRIVATE $<BUILD_INTERFACE:-Wall -Wextra -Wpedantic>)
        # The crashpad and breakpad headers generate the following warnings that we
        # ignore specifically
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE $<BUILD_INTERFACE:-Wno-variadic-macros -Wno-multichar>)
        else()
            target_compile_options(${target} PRIVATE $<BUILD_INTERFACE:-Wno-variadic-macros -Wno-gnu-include-next -Wno-multichar>)
        endif()
        # ignore all warnings for mpack
        set_source_files_properties(
                "${PROJECT_SOURCE_DIR}/vendor/mpack.c"
                PROPERTIES
                COMPILE_FLAGS
                "-w"
        )

        set(_SENTRY_PLATFORM_LIBS "dl" "rt")
    endfunction()

    function(sentry_platform_modify_crashpad_target target)
        # Unused
    endfunction()

    macro(sentry_platform_install)
        # Unused
    endmacro()

    function(sentry_platform_modify_example_target target)
        # Unused
    endfunction()
endif()