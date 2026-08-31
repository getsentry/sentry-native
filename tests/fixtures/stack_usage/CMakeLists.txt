sentry_get_property(SOURCES)
sentry_get_property(COMPILE_DEFINITIONS)
sentry_get_property(INTERFACE_INCLUDE_DIRECTORIES)
sentry_get_property(INCLUDE_DIRECTORIES)
sentry_get_property(LINK_LIBRARIES)
sentry_get_property(INTERFACE_LINK_LIBRARIES)

list(FILTER SENTRY_SOURCES EXCLUDE REGEX "\\.rc$")

add_executable(sentry_stack_usage
	${SENTRY_SOURCES}
	stack_usage.c
)

if(SENTRY_BACKEND_CRASHPAD)
	add_dependencies(sentry_stack_usage crashpad::handler)
	if(WIN32)
		add_dependencies(sentry_stack_usage crashpad::wer)
	endif()
endif()

if(SENTRY_BACKEND_NATIVE)
	add_dependencies(sentry_stack_usage sentry-crash)
	if(WIN32)
		add_dependencies(sentry_stack_usage sentry-wer)
	endif()
endif()

target_link_libraries(sentry_stack_usage PRIVATE
	${SENTRY_LINK_LIBRARIES}
	${SENTRY_INTERFACE_LINK_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:rt>"
)
target_compile_definitions(sentry_stack_usage PRIVATE
	${SENTRY_COMPILE_DEFINITIONS})
target_include_directories(sentry_stack_usage PRIVATE
	${SENTRY_INTERFACE_INCLUDE_DIRECTORIES}
	${SENTRY_INCLUDE_DIRECTORIES}
)

if(MSVC)
	target_compile_options(sentry_stack_usage PRIVATE
		$<BUILD_INTERFACE:/wd5105>)
endif()

if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
	set_property(TARGET sentry_stack_usage PROPERTY
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
