set(SENTRY_NATIVE_TEST_SOURCES
	main.c
	sentry_testsupport.h
	test_attachments.c
	test_basic.c
	test_consent.c
	test_database.c
	test_envelopes.c
	test_modulefinder.c
	test_path.c
	tests.inc
	test_slice.c
	test_symbolizer.c
	test_sync.c
	test_unwinder.c
	test_utils.c
	test_uuid.c
	test_value.c
)

# make paths absolute
PREPEND(SENTRY_NATIVE_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/" ${SENTRY_NATIVE_TEST_SOURCES})
target_sources(sentry_test_unit PRIVATE ${SENTRY_NATIVE_TEST_SOURCES})
