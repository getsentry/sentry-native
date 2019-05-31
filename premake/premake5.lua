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

  filter "system:windows"
    platforms {"Win64"}
    defines {"SENTRY_BUILD_SHARED"}
  filter {}

--- SENTRYPAD ---
include "premake5.sentrypad.lua"

--- CRASHPAD ---
include "premake5.crashpad.lua"

--- BREAKPAD ---
--include "premake5.breakpad.lua"
