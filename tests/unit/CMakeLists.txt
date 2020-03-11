get_target_property(SENTRY_NATIVE_ALL_SOURCES sentry SOURCES)
get_target_property(SENTRY_COMPILE_DEFINITIONS sentry COMPILE_DEFINITIONS)
get_target_property(SENTRY_INTERFACE_INCLUDE_DIRECTORIES sentry INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SENTRY_INCLUDE_DIRECTORIES sentry INCLUDE_DIRECTORIES)
get_target_property(SENTRY_LINK_LIBRARIES sentry LINK_LIBRARIES)
get_target_property(SENTRY_INTERFACE_LINK_LIBRARIES sentry INTERFACE_LINK_LIBRARIES)

add_executable(sentry_test_unit
	${SENTRY_NATIVE_ALL_SOURCES}
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

target_compile_definitions(sentry_test_unit PRIVATE ${SENTRY_COMPILE_DEFINITIONS})
target_include_directories(sentry_test_unit PRIVATE
	${SENTRY_INTERFACE_INCLUDE_DIRECTORIES}
	${SENTRY_INCLUDE_DIRECTORIES}
)
target_link_libraries(sentry_test_unit PRIVATE
	${SENTRY_LINK_LIBRARIES}
	${SENTRY_INTERFACE_LINK_LIBRARIES}
)

target_compile_definitions(sentry_test_unit PRIVATE SENTRY_UNITTEST)

add_test(NAME sentry_test_unit COMMAND sentry_test_unit)
