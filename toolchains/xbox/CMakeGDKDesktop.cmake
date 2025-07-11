# Adapted by Sentry from:
# https://github.com/microsoft/Xbox-GDK-Samples/blob/710f4bd9095d3796d07505249a7b383857e8a23f/Samples/Tools/CMakeExample/CMake/CMakeGDKDesktop.cmake
#
# CMakeGDKDesktop.cmake : CMake definitions for Microsoft GDK targeting PC
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GDK_DESKTOP_TOOLCHAIN_)
  return()
endif()

#--- Microsoft Game Development Kit

include("${CMAKE_CURRENT_LIST_DIR}/DetectGDK.cmake")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES GDK_VERSION BUILD_USING_BWOI)

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)

#--- GameRuntime and Extension Libraries
include(${CMAKE_CURRENT_LIST_DIR}/GDK-targets.cmake)

message("Microsoft GDK = ${Console_SdkRoot}/${GDK_VERSION}")

#--- Tools
find_program(MAKEPKG_TOOL makepkg.exe
    REQUIRED NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_DEFAULT_PATH
    HINTS "${Console_SdkRoot}/bin")

message("MGC Tool = ${MAKEPKG_TOOL}")

find_program(DIRECTX_DXC_TOOL dxc.exe
        REQUIRED NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_DEFAULT_PATH
        HINTS "${Console_SdkRoot}/${GDK_VERSION}")

message("DXC Compiler = ${DIRECTX_DXC_TOOL}")

set(_GDK_DESKTOP_TOOLCHAIN_ ON)
