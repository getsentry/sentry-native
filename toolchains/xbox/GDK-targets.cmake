# Adapted by Sentry from:
# https://github.com/microsoft/Xbox-GDK-Samples/blob/710f4bd9095d3796d07505249a7b383857e8a23f/Samples/Tools/CMakeExample/CMake/GDK-targets.cmake
#
# GDK-targets.cmake : Defines library imports for the Microsoft GDK shared libraries
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if(_GDK_TARGETS_)
  return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ERROR: Microsoft GDK only supports 64-bit")
endif()

if(NOT GDK_VERSION)
    message(FATAL_ERROR "ERROR: GDK_VERSION must be set")
endif()

#--- Locate Microsoft GDK
if(BUILD_USING_BWOI)
    if(DEFINED ENV{ExtractedFolder})
        cmake_path(SET ExtractedFolder "$ENV{ExtractedFolder}")
    else()
        set(ExtractedFolder "d:/xtrctd.sdks/BWOIExample/")
    endif()

    if(NOT EXISTS ${ExtractedFolder})
        message(FATAL_ERROR "ERROR: BWOI requires a valid ExtractedFolder (${ExtractedFolder})")
    endif()

    set(Console_SdkRoot "${ExtractedFolder}/Microsoft GDK")
else()
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;GRDKInstallPath]" ABSOLUTE CACHE)
endif()

if(NOT EXISTS "${Console_SdkRoot}/${GDK_VERSION}")
    message(FATAL_ERROR "ERROR: Cannot locate Microsoft Game Development Kit (GDK) - ${GDK_VERSION}")
endif()

#--- GameRuntime Library (for Xbox these are included in the Console_Libs variable)
if(NOT _GDK_XBOX_)
    add_library(Xbox::GameRuntime STATIC IMPORTED)
    set_target_properties(Xbox::GameRuntime PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${GDK_VERSION}/GRDK/gameKit/Lib/amd64/xgameruntime.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${GDK_VERSION}/GRDK/gameKit/Include"
        INTERFACE_COMPILE_FEATURES "cxx_std_11"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    if(GDK_VERSION GREATER_EQUAL 220600)
        add_library(Xbox::GameInput STATIC IMPORTED)
        set_target_properties(Xbox::GameInput PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${GDK_VERSION}/GRDK/gameKit/Lib/amd64/gameinput.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${GDK_VERSION}/GRDK/gameKit/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    endif()
endif()

#--- Extension Libraries
set(Console_GRDKExtLibRoot "${Console_SdkRoot}/${GDK_VERSION}/GRDK/ExtensionLibraries")
set(ExtensionPlatformToolset 142)

if(GDK_VERSION GREATER_EQUAL 241000)
   set(Console_GRDKExtIncludePath "Include")
   set(Console_GRDKExtLibPath "Lib/x64")
   set(Console_GRDKExtDLLPath "Redist/x64")
else()
   set(Console_GRDKExtIncludePath "DesignTime/CommonConfiguration/neutral/Include")
   set(Console_GRDKExtLibPath "DesignTime/CommonConfiguration/neutral/Lib")
   set(Console_GRDKExtDLLPath "Redist/CommonConfiguration/neutral")
endif()

# XCurl
add_library(Xbox::XCurl SHARED IMPORTED)
set_target_properties(Xbox::XCurl PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/${Console_GRDKExtDLLPath}/XCurl.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/${Console_GRDKExtLibPath}/XCurl.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/${Console_GRDKExtIncludePath}")

# Xbox.Services.API.C (requires XCurl)
add_library(Xbox::XSAPI STATIC IMPORTED)
set_target_properties(Xbox::XSAPI PROPERTIES
    IMPORTED_LOCATION_RELEASE "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/${Console_GRDKExtLibPath}/Release/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/${Console_GRDKExtLibPath}/Debug/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/${Console_GRDKExtIncludePath}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

# Xbox::HTTPClient (prior to June 2024 was included as part of Xbox.Services.API.C)
if(GDK_VERSION GREATER_EQUAL 240600)
    add_library(Xbox::HTTPClient SHARED IMPORTED)
    set_target_properties(Xbox::HTTPClient PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.LibHttpClient/${Console_GRDKExtDLLPath}/libHttpClient.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.LibHttpClient/${Console_GRDKExtLibPath}/libHttpClient.GDK.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.LibHttpClient/${Console_GRDKExtIncludePath}")
else()
    add_library(Xbox::HTTPClient STATIC IMPORTED)
    set_target_properties(Xbox::HTTPClient PROPERTIES
        IMPORTED_LOCATION_RELEASE "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/${Console_GRDKExtLibPath}/Release/v${ExtensionPlatformToolset}/libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib"
        IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/${Console_GRDKExtLibPath}/Debug/v${ExtensionPlatformToolset}/libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
endif()

target_link_libraries(Xbox::XSAPI INTERFACE Xbox::HTTPClient Xbox::XCurl appnotify.lib winhttp.lib crypt32.lib)

# GameChat2
add_library(Xbox::GameChat2 SHARED IMPORTED)
set_target_properties(Xbox::GameChat2 PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/${Console_GRDKExtDLLPath}/GameChat2.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/${Console_GRDKExtLibPath}/GameChat2.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/${Console_GRDKExtIncludePath}")

# PlayFab Multiplayer (requires XCurl)
add_library(Xbox::PlayFabMultiplayer SHARED IMPORTED)
set_target_properties(Xbox::PlayFabMultiplayer PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/${Console_GRDKExtDLLPath}/PlayFabMultiplayerGDK.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/${Console_GRDKExtLibPath}/PlayFabMultiplayerGDK.lib"
    IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/${Console_GRDKExtIncludePath}")

target_link_libraries(Xbox::PlayFabMultiplayer INTERFACE Xbox::XCurl)

# PlayFab Services (requires XCurl)
if(GDK_VERSION GREATER_EQUAL 230300)
    add_library(Xbox::PlayFabServices SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabServices PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Services.C/${Console_GRDKExtDLLPath}/PlayFabServices.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Services.C/${Console_GRDKExtLibPath}/PlayFabServices.GDK.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Services.C/${Console_GRDKExtIncludePath}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    add_library(Xbox::PlayFabCore SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabCore PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Services.C/${Console_GRDKExtDLLPath}/PlayFabCore.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Services.C/${Console_GRDKExtLibPath}/PlayFabCore.GDK.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    target_link_libraries(Xbox::PlayFabServices INTERFACE Xbox::PlayFabCore Xbox::XCurl)
endif()

# PlayFab Party
add_library(Xbox::PlayFabParty SHARED IMPORTED)
set_target_properties(Xbox::PlayFabParty PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/${Console_GRDKExtDLLPath}/Party.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/${Console_GRDKExtLibPath}/Party.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/${Console_GRDKExtIncludePath}")

# PlayFab Party Xbox LIVE (requires PlayFab Party)
add_library(Xbox::PlayFabPartyLIVE SHARED IMPORTED)
set_target_properties(Xbox::PlayFabPartyLIVE PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/${Console_GRDKExtDLLPath}/PartyXboxLive.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/${Console_GRDKExtLibPath}/PartyXboxLive.lib"
    IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::PlayFabParty
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/${Console_GRDKExtIncludePath}")

target_link_libraries(Xbox::PlayFabPartyLIVE INTERFACE Xbox::PlayFabParty)

set(_GDK_TARGETS_ ON)
