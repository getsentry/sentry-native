workspace "Sentrypad"

function sentrypad_common()
  language "C++"
  cppdialect "C++14"
  includedirs {
    SRC_ROOT.."/include",
  }

  filter "system:macosx or linux"
    toolset("clang")
  filter "system:windows"
    -- Some defines are missing in Windows SDK version 8.1, that's why we need "latest" here.
    -- Because of this, we have to run premake on Windows Machine, or even right on the target machine :(
    systemversion "latest"

    defines {
      "NOMINMAX",
      "UNICODE",
      "WIN32_LEAN_AND_MEAN",
      "_UNICODE",
      "_CRT_SECURE_NO_WARNINGS",
      -- "_HAS_EXCEPTIONS=0",
    }

  filter {}
end

project "sentry_crashpad"
  kind "SharedLib"
  sentrypad_common()

  defines {"SENTRY_CRASHPAD"}
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
    SRC_ROOT.."/src/backend/crashpad/**.cpp",
    SRC_ROOT.."/src/backend/crashpad/**.hpp",
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
    }
  filter "system:linux"
    links {
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
  buildoptions {
    "-fPIC",
  }
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
