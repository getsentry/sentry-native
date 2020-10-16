import sys
import os

is_android = os.environ.get("ANDROID_API")
is_x86 = os.environ.get("TEST_X86")
is_asan = "asan" in os.environ.get("RUN_ANALYZER", "")
is_kcov = "kcov" in os.environ.get("RUN_ANALYZER", "")
is_valgrind = "valgrind" in os.environ.get("RUN_ANALYZER", "")

has_http = not is_android and not (sys.platform == "linux" and is_x86)
# breakpad does not work correctly when using kcov or valgrind
has_breakpad = (
    not is_valgrind
    and not is_kcov
    and (sys.platform == "linux" or sys.platform == "win32")
)
# crashpad requires http, and doesn’t work with kcov/valgrind either
has_crashpad = has_http and not is_valgrind and not is_kcov and not is_android
# android has no local filesystem
has_files = not is_android
