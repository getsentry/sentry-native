import json
import os
import pytest
import re
from . import run
from .cmake import CMake

gbenchmarks = {}


def enumerate_unittests():
    regexp = re.compile(r"XX\((.*?)\)")
    # TODO: actually generate the `tests.inc` file with python
    curdir = os.path.dirname(os.path.realpath(__file__))
    with open(os.path.join(curdir, "unit/tests.inc"), "r") as testsfile:
        for line in testsfile:
            match = regexp.match(line)
            if match:
                yield match.group(1)


def pytest_generate_tests(metafunc):
    if "unittest" in metafunc.fixturenames:
        metafunc.parametrize("unittest", enumerate_unittests())


@pytest.fixture(scope="session")
def cmake(tmp_path_factory):
    cmake = CMake(tmp_path_factory)

    yield cmake.compile

    cmake.destroy()


def pytest_addoption(parser):
    parser.addoption(
        "--with_crashpad_wer",
        action="store_true",
        help="Enables tests for the crashpad WER module on Windows",
    )


def pytest_runtest_setup(item):
    if "with_crashpad_wer" in item.keywords and not item.config.getoption(
        "--with_crashpad_wer"
    ):
        pytest.skip("need --with_crashpad_wer to run this test")


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "with_crashpad_wer: mark test to only run when WER testing is enabled",
    )


@pytest.hookimpl(tryfirst=True)
def pytest_runtest_protocol(item, nextitem):
    pytest._current_test_name = item.nodeid


@pytest.fixture
def gbenchmark():
    def _load(json_path, test_name=None):
        if test_name is None:
            test_name = getattr(pytest, "_current_test_name", "unknown")

        with open(json_path, "r") as f:
            data = json.load(f)
        gbenchmarks[test_name] = data
        return data

    return _load


def pytest_terminal_summary(terminalreporter, exitstatus, config):
    if gbenchmarks:
        terminalreporter.write_sep("=", "benchmark results", green=True, bold=True)

        for test_name, data in gbenchmarks.items():
            terminalreporter.write(test_name)
            terminalreporter.currentfspath = 1
            terminalreporter.ensure_newline()

            for bm in data["benchmarks"]:
                if bm.get("skipped") == True:
                    terminalreporter.write_line(
                        f"  {bm['name']} skipped: {bm['skip_message']}"
                    )
                else:
                    terminalreporter.write_line(
                        f"  {bm['name']} real: {bm['real_time']:.3f}{bm['time_unit']}, cpu: {bm['cpu_time']:.3f}{bm['time_unit']}"
                    )
