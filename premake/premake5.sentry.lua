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
    }
    defines {
      "SENTRY_WITH_LIBCURL_TRANSPORT",
    }

  filter "system:windows"
    links {
      "winhttp.lib"
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

project "sentry_crashpad"
  kind "SharedLib"
  sentry_native_common()
  sentry_native_library()

  defines {
    "SENTRY_WITH_CRASHPAD_BACKEND"
  }
  includedirs {
    CRASHPAD_PKG,
    CRASHPAD_PKG.."/include",
    CRASHPAD_PKG.."/third_party/mini_chromium/mini_chromium",
  }

  -- Crashpad
  links {
    "crashpad_minichromium_base",
    "crashpad_client",
    "crashpad_util",
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
    BREAKPAD_PKG.."/include",
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

project "sentry_example"
  kind "ConsoleApp"
  sentry_native_common()

  links {
    "sentry",
  }

  files {
    SRC_ROOT.."/examples/sentry.c",
  }

project "sentry_example_crashpad"
  kind "ConsoleApp"
  sentry_native_common()

  links {
    "sentry_crashpad"
  }

  files {
    SRC_ROOT.."/examples/sentry_crashpad.c",
  }

project "sentry_example_breakpad"
  kind "ConsoleApp"
  sentry_native_common()

  links {
    "sentry_breakpad",
    "dl"
  }

  files {
    SRC_ROOT.."/examples/sentry_breakpad.c",
  }

project "sentry_tests"
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
