-- premake5.lua

newoption {
  trigger     = "src-root",
  description = "Sentrypad source root"
}

SRC_ROOT = (_OPTIONS["src-root"] or "..")

CRASHPAD_PKG = "../crashpad/build/crashpad"
BREAKPAD_PKG = "../breakpad/deps/breakpad"

workspace "Sentrypad"
  configurations {"Release", "Debug"}
  symbols "On"

  targetdir "bin/%{cfg.architecture}/%{cfg.buildcfg}"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"

  filter "system:windows"
    platforms {"x64", "Win32"}
    defines {"SENTRY_BUILD_SHARED"}

    -- Some defines are missing in Windows SDK version 8.1, that's why we need "latest" here.
    -- Because of this, we have to run premake on Windows Machine, or even right on the target machine :(
    systemversion "latest"

    defines {
      "NOMINMAX",
      "UNICODE",
      "WIN32_LEAN_AND_MEAN",
      "_CRT_SECURE_NO_WARNINGS",
      "_HAS_EXCEPTIONS=0",
      "_UNICODE",
    }

  filter {"system:macosx", "kind:ConsoleApp or SharedLib"}
    postbuildcommands {"dsymutil %{cfg.buildtarget.abspath}"}
  filter {}


--- SENTRYPAD ---
include "premake5.sentrypad.lua"

--- CRASHPAD ---
include "premake5.crashpad.lua"

--- BREAKPAD ---
--include "premake5.breakpad.lua"
