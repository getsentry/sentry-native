#include "sentry.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#    include <intrin.h>
#    define NOINLINE __declspec(noinline)
#    define CALLER_ADDRESS _ReturnAddress()
#else
#    define NOINLINE __attribute__((noinline))
#    define CALLER_ADDRESS                                                     \
        __builtin_extract_return_addr(__builtin_return_address(0))
#endif

class custom_exception : public std::exception {
public:
    const char *
    what() const noexcept override
    {
        return "custom exception";
    }
};

NOINLINE void
throw_exception()
{
    std::printf("THROW_CALLER:%p\n", CALLER_ADDRESS);
    std::fflush(stdout);
    throw std::runtime_error("something went wrong");
}

NOINLINE void
call_throw()
{
    try {
        throw_exception();
    } catch (...) {
        throw;
    }
}

int
main(int argc, char **argv)
{
    std::set_terminate([]() noexcept {
        std::puts("terminate_handler");
        std::fflush(stdout);
        std::abort();
    });

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, 1);
    // DMP takes precedent over C++ exception metadata
    sentry_options_set_crash_reporting_mode(
        options, SENTRY_CRASH_REPORTING_MODE_NATIVE);
    sentry_options_set_on_crash(
        options,
        [](const sentry_ucontext_t *, sentry_value_t event, sentry_hint_t *,
            void *) {
            sentry_value_t exceptions
                = sentry_value_get_by_key(event, "exception");
            sentry_value_t values
                = sentry_value_get_by_key(exceptions, "values");
            sentry_value_t exception = sentry_value_get_by_index(values, 0);
            const char *type = sentry_value_as_string(
                sentry_value_get_by_key(exception, "type"));
            const char *value = sentry_value_as_string(
                sentry_value_get_by_key(exception, "value"));

            std::printf("%s\n%s\n", type ? type : "", value ? value : "");
            std::fflush(stdout);
            return event;
        },
        nullptr);
    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    auto has_arg = [argc, argv](std::string_view arg) {
        return std::any_of(argv + 1, argv + argc,
            [arg](const char *value) { return value == arg; });
    };

    if (has_arg("e2e-test")) {
        char test_id[37];
        sentry_uuid_t test_uuid = sentry_uuid_new_v4();
        sentry_uuid_as_string(&test_uuid, test_id);
        sentry_set_tag("test.id", test_id);
        sentry_set_tag("test.suite", "e2e");
        std::printf("TEST_ID:%s\n", test_id);
        std::fflush(stdout);
    }

    if (has_arg("int")) {
        throw 42;
    }
    if (has_arg("string")) {
        throw std::string("uncaught string");
    }
    if (has_arg("runtime_error")) {
        throw std::runtime_error("something went wrong");
    }
    if (has_arg("out_of_range")) {
        (void)std::string().at(0);
    }
    if (has_arg("invalid_argument")) {
        (void)std::stoi("not a number");
    }
    if (has_arg("custom_exception")) {
        throw custom_exception();
    }
    if (has_arg("range_error")) {
        throw std::range_error(std::string(2048, 'x'));
    }
    if (has_arg("throw_stack") || has_arg("rethrow_stack")) {
        try {
            call_throw();
        } catch (...) {
            if (has_arg("rethrow_stack")) {
                try {
                    throw std::runtime_error("handled exception");
                } catch (...) {
                }
                throw;
            }
            std::terminate();
        }
    }
    if (has_arg("reused_stack")) {
        try {
            throw_exception();
        } catch (...) {
        }
        std::rethrow_exception(
            std::make_exception_ptr(std::runtime_error("fresh")));
    }
    return EXIT_SUCCESS;
}
