# Sentry Native Backend
# Lightweight, portable crash backend for all platforms

# Allow target_link_libraries to link to targets from parent directories
cmake_policy(SET CMP0079 NEW)

set(SENTRY_BACKEND_NATIVE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/minidump/sentry_minidump_format.h
    ${CMAKE_CURRENT_SOURCE_DIR}/minidump/sentry_minidump_writer.h
)

# Crash handler and IPC
list(APPEND SENTRY_BACKEND_NATIVE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sentry_crash_ipc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sentry_crash_daemon.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sentry_crash_handler.c
)

# Platform-specific minidump writers
if(LINUX OR ANDROID)
    list(APPEND SENTRY_BACKEND_NATIVE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/minidump/sentry_minidump_linux.c
    )
elseif(APPLE)
    list(APPEND SENTRY_BACKEND_NATIVE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/minidump/sentry_minidump_macos.c
    )
elseif(WIN32)
    list(APPEND SENTRY_BACKEND_NATIVE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/minidump/sentry_minidump_windows.c
    )
endif()

# Add sources to sentry library
target_sources(sentry PRIVATE ${SENTRY_BACKEND_NATIVE_SOURCES})

# Add include directory for native backend headers
target_include_directories(sentry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Platform-specific libraries
if(LINUX OR ANDROID)
    # Linux needs pthread and rt for shared memory
    target_link_libraries(sentry PRIVATE pthread rt)
elseif(APPLE)
    # macOS needs CoreFoundation and Security frameworks
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(SECURITY_LIBRARY Security REQUIRED)
    target_link_libraries(sentry PRIVATE
        ${COREFOUNDATION_LIBRARY}
        ${SECURITY_LIBRARY}
    )
elseif(WIN32)
    # Windows needs dbghelp for MiniDumpWriteDump
    target_link_libraries(sentry PRIVATE dbghelp)
endif()

# Enable C11 for atomics support
set_property(TARGET sentry PROPERTY C_STANDARD 11)

message(STATUS "Sentry Native Backend: Enabled")
message(STATUS "  - Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  - Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  - Backend: Native (lightweight, ~5K LOC)")
