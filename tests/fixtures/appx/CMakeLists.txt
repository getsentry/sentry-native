cmake_minimum_required(VERSION 3.10)
project(sentry_example_appx LANGUAGES C)

if(WIN32 AND NOT XBOX)
    include(CheckSymbolExists)
    check_symbol_exists(GetCurrentPackageFullName "windows.h;appmodel.h" HAVE_APPMODEL)

    add_executable(sentry_example_appx
        ${SENTRY_SOURCE_DIR}/examples/example.c
    )
    target_link_libraries(sentry_example_appx PRIVATE sentry)
    if(HAVE_APPMODEL)
        target_compile_definitions(sentry_example_appx PRIVATE HAVE_APPMODEL)
    endif()

    if(MSVC)
        target_sources(sentry_example_appx PRIVATE sentry_example_appx.manifest)
        target_compile_options(sentry_example_appx PRIVATE /wd5105 /wd4717)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(sentry_example_appx PRIVATE -Wno-infinite-recursion /Od)
        endif()

        target_compile_options(sentry_example_appx PRIVATE /guard:cf)
        target_link_options(sentry_example_appx PRIVATE /GUARD:CF)
    else()
        target_sources(sentry_example_appx PRIVATE sentry_example_appx.rc)
    endif()

    if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
        set_property(TARGET sentry_example_appx PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    set_target_properties(sentry_example_appx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
    )

    add_custom_command(TARGET sentry_example_appx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/AppxManifest.xml"
            "$<TARGET_FILE_DIR:sentry_example_appx>/AppxManifest.xml"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/classicAppCompat.sccd"
            "$<TARGET_FILE_DIR:sentry_example_appx>/classicAppCompat.sccd")
endif()
