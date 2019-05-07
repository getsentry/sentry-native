-- premake5.lua
SRC_ROOT = "../"

DEPS_DIR="./deps"
-- XXX Fix this on Windows
PLATFORM = io.popen('uname -s','r'):read('*l')
ARCH = io.popen('uname -m','r'):read('*l')

if PLATFORM == 'Darwin' or PLATFORM == 'Linux' or PLATFORM == 'Windows' then
  print('Platform: '..PLATFORM)
  CRASHPAD_PKG = DEPS_DIR.."/crashpad-"..PLATFORM
  BREAKPAD_PKG = DEPS_DIR.."/breakpad-"..PLATFORM
else
  print('Unsupported platform, exiting.')
  os.exit(1)
end

function file_exists(name)
  local f = io.open(name, "r")
  return f ~= nil and io.close(f)
end

workspace "sentrypad"
  configurations {"Release"}
  language "C++"
  cppdialect "C++14"
  includedirs {
    SRC_ROOT.."/include",
  }

  filter "system:macosx or linux"
    toolset("clang")
  filter {}


project "download_crashpad"
  kind "Makefile"
  buildcommands {
    "IF_NOT_EXIST=1 bash ./scripts/download.sh crashpad"
  }
  --- FIXME: Is there a way to tell "make" here that we don't want to redownload that target?
  cleancommands {
    "rm -rf "..CRASHPAD_PKG,
  }

project "download_breakpad"
  kind "Makefile"
  buildcommands {
    "IF_NOT_EXIST=1 bash ./scripts/download.sh breakpad"
  }
  cleancommands {
    "rm -rf "..BREAKPAD_PKG,
  }

project "sentry_crashpad"
  kind "SharedLib"
  defines {"SENTRY_CRASHPAD"}
  dependson {"download_crashpad"}
  includedirs {
    CRASHPAD_PKG.."/include",
    CRASHPAD_PKG.."/include/mini_chromium",
  }

  libdirs {
    CRASHPAD_PKG.."/lib",
  }

  files {
    SRC_ROOT.."/src/sentry.cpp",
    SRC_ROOT.."/src/crashpad_backend.cpp",
    SRC_ROOT.."/src/vendor/mpack.c",
  }

  postbuildcommands {
    "{COPY} "..CRASHPAD_PKG.."/bin/crashpad_handler %{cfg.buildtarget.directory}",
  }

  -- Crashpad
  links {
    "client", "base", "util"
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



project "sentry_breakpad"
  kind "SharedLib"
  defines {"SENTRY_BREAKPAD"}
  dependson {"download_breakpad"}
  buildoptions {
    "-fvisibility=hidden",
  }
  includedirs {
    BREAKPAD_PKG.."/include",
  }

  libdirs {
    BREAKPAD_PKG.."/lib",
  }

  files {
    SRC_ROOT.."/src/sentry.cpp",
    SRC_ROOT.."/src/breakpad_backend.cpp",
    SRC_ROOT.."/src/vendor/mpack.c",
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
		files {
			SRC_ROOT.."/src/breakpad_macuploader.mm",
		}
  filter "system:linux"
    -- System
    links {
      "pthread",
    }
  filter {}

project "example_crashpad"
  kind "ConsoleApp"
  links {"sentry_crashpad"}
  buildoptions {
    "-fPIC",
  }
  files {
    SRC_ROOT.."/example.c",
  }


project "example_breakpad"
  kind "ConsoleApp"
  links {"sentry_breakpad", "dl"}
  buildoptions {
    "-fPIC",
  }
  files {
    SRC_ROOT.."/example.c",
  }
