workspace "Sentry-Native"

function sentry_native_common()
  language "C++"
  cppdialect "C++14"

  includedirs {
    SRC_ROOT.."/include",
  }

  pic "on"

  filter "system:macosx or linux"
    toolset("clang")

  filter "system:windows"
    buildoptions {
      "/wd4201",  -- nonstandard extension used : nameless struct/union
    }

  filter {}
end

-- Temporary disable specific projects for Android by adding this function call
function disable_for_android()
  filter "system:android"
    kind "SharedLib"
    removefiles {
      SRC_ROOT.."/**",
    }
  filter {}
end

function sentry_native_library()
  libdirs {
    "bin/Release",
  }

  files {
    SRC_ROOT.."/src/**.c",
    SRC_ROOT.."/src/**.h",
    SRC_ROOT.."/src/**.cpp",
    SRC_ROOT.."/src/**.hpp",
  }

  filter "system:macosx"
    links {
      "curl",
    }
    defines {
      "SENTRY_WITH_LIBCURL_TRANSPORT",
    }

  filter "system:linux"
    links {
      "pthread",
      "uuid",
      "curl",
      "dl",
    }
    defines {
      "SENTRY_WITH_LIBCURL_TRANSPORT",
    }

  filter "system:windows"
    links {
      "winhttp.lib",
      "dbghelp.lib",
      "rpcrt4.lib",  -- for UUID operations
    }
    defines {
      "SENTRY_WITH_WINHTTP_TRANSPORT",
    }

  filter {}
end

project "sentry"
  kind "SharedLib"
  sentry_native_common()
  sentry_native_library()

  -- the android unwinder needs cmake.  make this work with premake as
  -- a fallback for now.
  filter "system:android"
    defines {
      "SENTRY_WITH_NULL_UNWINDER"
    }

project "sentry_crashpad"
  kind "SharedLib"
  sentry_native_common()
  sentry_native_library()

  dependson {
    "crashpad_handler"
  }

  defines {
    "SENTRY_WITH_CRASHPAD_BACKEND"
  }

  includedirs {
    CRASHPAD_PKG,
    CRASHPAD_PKG.."/third_party/mini_chromium/mini_chromium",
  }

  -- Crashpad
  links {
    "crashpad_client",
    "crashpad_util",
    "crashpad_minichromium_base",
  }

  filter "system:macosx"
    -- System
    links {
      "Foundation.framework",
      "Security.framework",
      "CoreText.framework",
      "CoreGraphics.framework",
      "IOKit.framework",
      "bsm",
    }

  filter {}

  disable_for_android()

project "sentry_breakpad"
  kind "SharedLib"
  sentry_native_common()
  sentry_native_library()

  defines {
    "SENTRY_WITH_BREAKPAD_BACKEND"
  }
  buildoptions {
    "-fvisibility=hidden",
  }
  includedirs {
    BREAKPAD_PKG.."/src",
  }

  -- Breakpad
  links {
    "breakpad_client",
  }

  filter "system:macosx"
    -- System
    links {
      "Foundation.framework",
      "pthread",
    }
    defines {
    }

  filter {}

project "example"
  kind "ConsoleApp"
  sentry_native_common()

  links {
    "sentry",
  }

  files {
    SRC_ROOT.."/examples/example.c",
  }

  -- make sure we have a build-id
  filter "system:linux"
    linkoptions { "-Wl,--build-id=uuid" }

  filter {}

  disable_for_android()

project "example_crashpad"
  kind "ConsoleApp"
  sentry_native_common()

  links {
    "sentry_crashpad"
  }

  dependson {
    "crashpad_handler"
  }

  files {
    SRC_ROOT.."/examples/example_crashpad.c",
  }

  disable_for_android()

project "example_breakpad"
  kind "ConsoleApp"
  sentry_native_common()

  links {
    "sentry_breakpad",
  }

  files {
    SRC_ROOT.."/examples/example_breakpad.c",
  }

  disable_for_android()

project "test_sentry"
  kind "ConsoleApp"
  sentry_native_common()
  -- We compile the exe, but with the library settings
  sentry_native_library()

  includedirs {
    SRC_ROOT.."/src",
  }
  defines {
    "SENTRY_WITH_TESTS"
  }

  files {
    SRC_ROOT.."/tests/**.cpp",
  }

  -- make sure we have a build-id
  filter "system:linux"
    -- -E is needed on linux to make dladdr work on the main executable
    linkoptions { "-Wl,--build-id=uuid,-E" }

  filter "system:android"

  filter {}
