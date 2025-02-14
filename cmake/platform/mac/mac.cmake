if(APPLE AND NOT IOS)
    macro(sentry_platform_setup)
        # Unused
    endmacro()

    macro(sentry_platform_init)
        sentry_link_pthread()

        set(SENTRY_DEFAULT_TRANSPORT "curl")

        set(SENTRY_DEFAULT_BACKEND "crashpad")

        foreach(lang ASM C CXX)
            string(REPLACE "-O2" "-O3" CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}")
        endforeach()

        find_program(DSYMUTIL_PROGRAM dsymutil)
        if(DSYMUTIL_PROGRAM)
            foreach(lang C CXX)
                foreach(var LINK_EXECUTABLE CREATE_SHARED_LIBRARY)
                    set(CMAKE_${lang}_${var} "${CMAKE_${lang}_${var}}" "${DSYMUTIL_PROGRAM} <TARGET>")
                endforeach()
            endforeach()
        endif()
    endmacro()

    function(sentry_platform_modify_target target)
        if(SENTRY_BUILD_SHARED_LIBS)
            sentry_install(FILES "$<TARGET_FILE:sentry>.dSYM" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        endif ()

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