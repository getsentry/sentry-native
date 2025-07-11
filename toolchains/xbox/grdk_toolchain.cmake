# Adapted by Sentry from:
# https://github.com/microsoft/Xbox-GDK-Samples/blob/710f4bd9095d3796d07505249a7b383857e8a23f/Samples/Tools/CMakeGDKExample/grdk_toolchain.cmake
#
# grdk_toolchain.cmake : CMake Toolchain file for Gaming.Desktop.x64
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GRDK_TOOLCHAIN_)
  return()
endif()

# Microsoft Game Development Kit

include("${CMAKE_CURRENT_LIST_DIR}/DetectGDK.cmake")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES GDK_VERSION)

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)

set(CMAKE_GENERATOR_PLATFORM "Gaming.Desktop.x64" CACHE STRING "" FORCE)
set(CMAKE_VS_PLATFORM_NAME "Gaming.Desktop.x64" CACHE STRING "" FORCE)

# Ensure our platform toolset is x64
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64" CACHE STRING "" FORCE)

# Let the GDK MSBuild rules decide the WindowsTargetPlatformVersion
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "" CACHE STRING "" FORCE)

if(${CMAKE_VERSION} GREATER_EQUAL "3.30")
    set(CMAKE_VS_USE_DEBUG_LIBRARIES "$<CONFIG:Debug>" CACHE STRING "" FORCE)
endif()

# Sets platform defines
set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_CXX_FLAGS_INIT} -D_GAMING_DESKTOP -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP" CACHE STRING "" FORCE)

# Add GDK props file
file(GENERATE OUTPUT gdk_build.props INPUT ${CMAKE_CURRENT_LIST_DIR}/gdk_build.props)

function(add_executable target_name)
  _add_executable(${target_name} ${ARGN})
  set_target_properties(${target_name} PROPERTIES VS_USER_PROPS gdk_build.props)
endfunction()

function(add_library target_name)
  _add_library(${target_name} ${ARGN})
  set_target_properties(${target_name} PROPERTIES VS_USER_PROPS gdk_build.props)
endfunction()

# Find DXC compiler
if(NOT GDK_DXCTool)
  set(GDK_DXCTool "dxc.exe")
  mark_as_advanced(GDK_DXCTool)
endif()

# The MicrosoftGame.Config file needs to be in the directory with the project to enable deploy for Gaming.Desktop.x64
file(GENERATE OUTPUT MicrosoftGame.Config INPUT ${CMAKE_CURRENT_LIST_DIR}/MicrosoftGameConfig.mgc)

set(_GRDK_TOOLCHAIN_ ON)
