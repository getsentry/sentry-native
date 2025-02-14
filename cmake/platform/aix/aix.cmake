if(CMAKE_SYSTEM_NAME STREQUAL "AIX" OR CMAKE_SYSTEM_NAME STREQUAL "OS400")
    macro(sentry_platform_setup)
        # Unused
    endmacro()

    macro(sentry_platform_init)
        set(AIX TRUE)

        sentry_link_pthread()

        set(SENTRY_DEFAULT_TRANSPORT "curl")

        set(SENTRY_DEFAULT_BACKEND "inproc")

        foreach(lang ASM C CXX)
            string(REPLACE "-O2" "-O3" CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}")
        endforeach()
    endmacro()

    function(sentry_platform_modify_target target)
        target_link_libraries(${target} PRIVATE m)

        # On IBM i PASE, flock is in libutil. Here because "sentry" exists now.
        if(CMAKE_SYSTEM_NAME STREQUAL "OS400")
            target_link_libraries(${target} PRIVATE util)
        endif()

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