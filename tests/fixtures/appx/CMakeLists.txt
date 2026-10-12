cmake_minimum_required(VERSION 3.10)
project(sentry_test_appx LANGUAGES C)

if(WIN32 AND NOT XBOX)
    include(CheckSymbolExists)
    check_symbol_exists(GetCurrentPackageFullName "windows.h;appmodel.h" HAVE_APPMODEL)

    add_executable(sentry_test_appx
        ${SENTRY_SOURCE_DIR}/tests/fixtures/integration.c
    )
    target_link_libraries(sentry_test_appx PRIVATE sentry)
    if(HAVE_APPMODEL)
        target_compile_definitions(sentry_test_appx PRIVATE HAVE_APPMODEL)
    endif()

    if(MSVC)
        target_sources(sentry_test_appx PRIVATE sentry_test_appx.manifest)
        target_compile_options(sentry_test_appx PRIVATE /wd5105 /wd4717)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(sentry_test_appx PRIVATE -Wno-infinite-recursion /Od)
        endif()

        target_compile_options(sentry_test_appx PRIVATE /guard:cf)
        target_link_options(sentry_test_appx PRIVATE /GUARD:CF)
    else()
        target_sources(sentry_test_appx PRIVATE sentry_test_appx.rc)
    endif()

    if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
        set_property(TARGET sentry_test_appx PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    set_target_properties(sentry_test_appx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
    )

    add_custom_command(TARGET sentry_test_appx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/AppxManifest.xml"
            "$<TARGET_FILE_DIR:sentry_test_appx>/AppxManifest.xml"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/classicAppCompat.sccd"
            "$<TARGET_FILE_DIR:sentry_test_appx>/classicAppCompat.sccd")
endif()
