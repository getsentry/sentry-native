if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Gaming.Xbox.Scarlett.x64")
    macro(sentry_platform_setup)
        # Unused

        set(SENTRY_TOOLCHAINS_DIR "${CMAKE_CURRENT_LIST_DIR}/toolchains")
        include("${SENTRY_TOOLCHAINS_DIR}/xbox/CMakeGDKScarlett.cmake")
    endmacro()

    macro(sentry_platform_init)
        set(SENTRY_DEFAULT_TRANSPORT "none")

        if(MSVC)
            option(SENTRY_BUILD_RUNTIMESTATIC "Build sentry-native with static runtime" OFF)

            set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} /utf-8")
            set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /utf-8")
        endif()

        set(SENTRY_DEFAULT_BACKEND "breakpad")

    endmacro()

    function(sentry_platform_modify_target target)
        set_target_properties(${target} PROPERTIES VS_USER_PROPS gdk_build.props)

        if(SENTRY_BUILD_SHARED_LIBS AND MSVC)
            sentry_install(FILES "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:$<TARGET_PDB_FILE:sentry>>"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}")
        endif()

        if(MSVC)
            if(CMAKE_SIZEOF_VOID_P EQUAL 4)
                set(CMAKE_ASM_MASM_FLAGS "${CMAKE_ASM_MASM_FLAGS} /safeseh")
            endif()

            target_compile_options(${target} PRIVATE $<BUILD_INTERFACE:/W4 /wd5105>)
            # ignore all warnings for mpack
            set_source_files_properties(
                    "${PROJECT_SOURCE_DIR}/vendor/mpack.c"
                    PROPERTIES
                    COMPILE_FLAGS
                    "/W0"
            )

            # set static runtime if enabled
            if(SENTRY_BUILD_RUNTIMESTATIC)
                set_property(TARGET sentry PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
            endif()
        else()
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
        endif ()

        if(MSVC AND CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
            #force WINNT to 5.1 for Windows XP toolchain
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0501")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^10")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0A00")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.3")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0603")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.2")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0602")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.1")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0601")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.0")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0600")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^5.2")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0502")
        elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^5.1")
            target_compile_definitions(${target} PRIVATE "_WIN32_WINNT=0x0501")
        endif()

        if(SENTRY_LINK_PTHREAD)
            list(APPEND _SENTRY_PLATFORM_LIBS "Threads::Threads")
        endif()

        set(_SENTRY_PLATFORM_LIBS "version")
    endfunction()

    function(sentry_platform_modify_crashpad_target target)
        if(WIN32 AND MSVC)
            sentry_install(FILES $<TARGET_PDB_FILE:crashpad_handler>
                    DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
            sentry_install(FILES $<TARGET_PDB_FILE:crashpad_wer>
                    DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
        endif()
    endfunction()

    macro(sentry_platform_install)
        if(MSVC AND SENTRY_BUILD_SHARED_LIBS)
            sentry_install(FILES $<TARGET_PDB_FILE:sentry>
                    DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
        endif()
    endmacro()

    function(sentry_platform_modify_example_target target)
        set_target_properties(${target} PROPERTIES VS_USER_PROPS gdk_build.props)
    endfunction()
endif()