-- premake5.lua

newoption {
  trigger     = "src-root",
  description = "Sentrypad source root"
}

SRC_ROOT = (_OPTIONS["src-root"] or "..")

CRASHPAD_PKG = "../crashpad/build/crashpad"

workspace "Sentrypad"
  configurations {"Release"}

--- SENTRYPAD ---
include "premake5.sentrypad.lua"

--- CRASHPAD ---
include "premake5.crashpad.lua"
