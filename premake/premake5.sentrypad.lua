workspace "Sentrypad"

function sentrypad_common()
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
  sentrypad_common()

  defines {"SENTRY_WITH_CRASHPAD_BACKEND"}
  includedirs {
    CRASHPAD_PKG,
    CRASHPAD_PKG.."/include",
    CRASHPAD_PKG.."/third_party/mini_chromium/mini_chromium",
  }

  libdirs {
    "bin/Release",
  }

  files {
    SRC_ROOT.."/src/**.cpp",
    SRC_ROOT.."/src/**.hpp",
    SRC_ROOT.."/src/vendor/mpack.c",
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
      "SENTRY_WITH_LIBCURL_TRANSPORT"
    }
  filter "system:linux"
    links {
    }
  filter "system:windows"
    links {
    }
    defines {
      "SENTRY_WITH_WINHTTP_TRANSPORT"
    }

-- project "sentry_breakpad"
--   kind "SharedLib"
--   defines {"SENTRY_BREAKPAD"}
--   buildoptions {
--     "-fvisibility=hidden",
--   }
--   includedirs {
--     BREAKPAD_PKG.."/include",
--   }

--   libdirs {
--     BREAKPAD_PKG.."/lib",
--   }

--   files {
--     SRC_ROOT.."/src/sentry.cpp",
--     SRC_ROOT.."/src/breakpad_backend.cpp",
--     SRC_ROOT.."/src/vendor/mpack.c",
--   }

--   -- Breakpad
--   links {
--     "breakpad_client",
--   }

--   filter "system:macosx"
--     -- System
--     links {
--       "Foundation.framework",
--       "pthread",
--     }
--   filter "system:linux"
--     -- System
--     links {
--       "pthread",
--     }
--   filter {}

project "sentry_example_crashpad"
  kind "ConsoleApp"
  sentrypad_common()

  links {"sentry_crashpad"}

  files {
    SRC_ROOT.."/example.c",
  }

-- project "sentry_example_breakpad"
--   kind "ConsoleApp"
--   sentrypad_common()

--   links {"sentry_breakpad", "dl"}
--   buildoptions {
--     "-fPIC",
--   }
--   files {
--     SRC_ROOT.."/example.c",
--   }
