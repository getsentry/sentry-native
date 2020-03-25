import pytest
import re
from . import cmake, run


def enumerate_unittests():
    regexp = re.compile("XX\((.*?)\)")
    # TODO: actually generate the `tests.inc` file with python
    with open("tests/unit/tests.inc", "r") as testsfile:
        for line in testsfile:
            match = regexp.match(line)
            if match:
                yield match.group(1)


def pytest_generate_tests(metafunc):
    if "unittest" in metafunc.fixturenames:
        metafunc.parametrize("unittest", enumerate_unittests())


class Unittests:
    def __init__(self, dir):
        # for unit tests, the backend does not matter, and we want to keep
        # the compile-times down
        cmake(
            dir,
            ["sentry_test_unit"],
            {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
        )
        self.dir = dir

    def run(self, test):
        run(self.dir, "sentry_test_unit", ["--no-summary", test], check=True)


@pytest.fixture(scope="session")
def unittests(tmp_path_factory):
    tmpdir = tmp_path_factory.mktemp("unittests")
    return Unittests(tmpdir)
