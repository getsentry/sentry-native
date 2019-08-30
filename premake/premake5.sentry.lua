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

project "sentry_crashpad"
  kind "SharedLib"
  sentry_native_common()

  defines {
    "SENTRY_WITH_CRASHPAD_BACKEND"
  }
  includedirs {
    CRASHPAD_PKG,
    CRASHPAD_PKG.."/include",
    CRASHPAD_PKG.."/third_party/mini_chromium/mini_chromium",
  }

  libdirs {
    "bin/Release",
  }

  files {
    SRC_ROOT.."/src/**.c",
    SRC_ROOT.."/src/**.h",
    SRC_ROOT.."/src/**.cpp",
    SRC_ROOT.."/src/**.hpp",
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
			"curl",
    }
    defines {
      "SENTRY_WITH_LIBCURL_TRANSPORT",
      "SENTRY_WITH_DARWIN_MODULE_FINDER",
    }
  filter "system:linux"
    links {
      "pthread",
      "uuid"
    }
    defines {
      "SENTRY_WITH_LINUX_MODULE_FINDER",
    }
  filter "system:windows"
    links {
    }
    defines {
      "SENTRY_WITH_WINHTTP_TRANSPORT"
    }

project "sentry_breakpad"
  kind "SharedLib"
  sentry_native_common()

  defines {"SENTRY_WITH_BREAKPAD_BACKEND"}
  buildoptions {
    "-fvisibility=hidden",
  }
  includedirs {
    BREAKPAD_PKG.."/include",
  }

  libdirs {
    "bin/Release"
  }

  files {
    SRC_ROOT.."/src/**.c",
    SRC_ROOT.."/src/**.h",
    SRC_ROOT.."/src/**.cpp",
    SRC_ROOT.."/src/**.hpp",
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
  filter "system:linux"
    -- System
    links {
      "pthread",
      "uuid",
    }
    defines {
      "SENTRY_WITH_LINUX_MODULE_FINDER",
    }
  filter {}

project "sentry_example_crashpad"
  kind "ConsoleApp"
  sentry_native_common()

  links {"sentry_crashpad"}

  files {
    SRC_ROOT.."/examples/sentry_crashpad.c",
  }

project "sentry_example_breakpad"
  kind "ConsoleApp"
  sentry_native_common()

  links {"sentry_breakpad", "dl"}

  files {
    SRC_ROOT.."/examples/sentry_breakpad.c",
  }
