import pytest
import sys
import os
from . import cmake

# TODO:
# * with crashpad backend:
#   - breadcrumbs, attachments, etc
#   - crash
#   - expect report via http

@pytest.mark.skipif(sys.platform == "linux" or os.environ.get("ANDROID_API"), reason="crashpad not supported on linux")
def test_crashpad_build(tmp_path):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND":"crashpad", "SENTRY_TRANSPORT":"none"})
