# Keep filesystem fixtures independent across cases, configurations, repeated
# runs, and concurrent CTest invocations. Preserve failed fixtures for diagnosis.
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef test_id)
set(working_directory "${TEST_WORKING_ROOT}/${TEST_NAME}-${test_id}")
file(MAKE_DIRECTORY "${working_directory}")
# path_basics intentionally checks that the current directory is nonempty.
file(WRITE "${working_directory}/.ctest-fixture" "Sentry unit-test fixture\n")

set(test_command ${TEST_LAUNCHER} ${TEST_EMULATOR} "${TEST_EXECUTABLE}")
if(TEST_INDIVIDUAL)
	list(APPEND test_command --no-summary "${TEST_NAME}")
endif()

execute_process(COMMAND ${test_command}
	WORKING_DIRECTORY "${working_directory}"
	RESULT_VARIABLE result)
if(NOT "${result}" STREQUAL "0")
	message(FATAL_ERROR "${TEST_NAME} failed (${result}); fixtures: ${working_directory}")
endif()

file(REMOVE_RECURSE "${working_directory}")
