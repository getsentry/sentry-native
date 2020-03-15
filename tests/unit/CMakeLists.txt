function(sentry_get_property NAME)
	get_target_property(prop sentry "${NAME}")
	if(NOT prop)
		set(prop)
	endif()
	set("SENTRY_${NAME}" "${prop}" PARENT_SCOPE)
endfunction()

sentry_get_property(SOURCES)
sentry_get_property(COMPILE_DEFINITIONS)
sentry_get_property(INTERFACE_INCLUDE_DIRECTORIES)
sentry_get_property(INCLUDE_DIRECTORIES)
sentry_get_property(LINK_LIBRARIES)
sentry_get_property(INTERFACE_LINK_LIBRARIES)

add_executable(sentry_test_unit
	${SENTRY_SOURCES}
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

# FIXME: cmake 3.13 introduced target_link_options
target_compile_definitions(sentry_test_unit PRIVATE ${SENTRY_COMPILE_DEFINITIONS})
target_include_directories(sentry_test_unit PRIVATE
	${SENTRY_INTERFACE_INCLUDE_DIRECTORIES}
	${SENTRY_INCLUDE_DIRECTORIES}
)
target_link_libraries(sentry_test_unit PRIVATE
	${SENTRY_LINK_LIBRARIES}
	${SENTRY_INTERFACE_LINK_LIBRARIES}
	"$<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Android>>:-Wl,-E>"
)

if(MINGW)
	target_compile_options(sentry_test_unit PRIVATE -Wno-unused-variable)
endif()

target_compile_definitions(sentry_test_unit PRIVATE SENTRY_UNITTEST)

add_test(NAME sentry_test_unit COMMAND sentry_test_unit)
