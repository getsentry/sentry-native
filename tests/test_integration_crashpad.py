import pytest
from .conditions import has_crashpad
from . import cmake

# TODO:
# * with crashpad backend:
#   - breadcrumbs, attachments, etc
#   - crash
#   - expect report via http

@pytest.mark.skipif(not has_crashpad, reason="test needs crashpad backend")
def test_crashpad_build(tmp_path):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND":"crashpad", "SENTRY_TRANSPORT":"none"})
