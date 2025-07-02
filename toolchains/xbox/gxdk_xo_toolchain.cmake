# Adapted by Sentry from:
# https://github.com/microsoft/Xbox-GDK-Samples/blob/aa45b831e7a71160a69a7d13e9d74844dc6aa210/Samples/Tools/CMakeGDKExample/gxdk_xs_toolchain.cmake
#
# gxdk_xo_toolchain.cmake : CMake Toolchain file for Gaming.Xbox.XboxOne.x64
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

include("${CMAKE_CURRENT_LIST_DIR}/gxdk_base_toolchain.cmake")

set(CMAKE_GENERATOR_PLATFORM "Gaming.Xbox.XboxOne.x64" CACHE STRING "" FORCE)
set(CMAKE_VS_PLATFORM_NAME "Gaming.Xbox.XboxOne.x64" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} -D_GAMING_XBOX_XBOXONE ${CMAKE_CXX_FLAGS_INIT}" CACHE STRING "" FORCE)
