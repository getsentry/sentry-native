import sys
import os

is_android = os.environ.get("ANDROID_API")
is_x86 = os.environ.get("TEST_X86")

has_crashpad = sys.platform != "linux" and not is_android
# 32-bit linux has no proper curl support
has_http = not is_android and not is_x86
has_inproc = True
has_breakpad = sys.platform == "linux" or sys.platform == "win32"
# android has no local filesystem
has_files = not is_android
