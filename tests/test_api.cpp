#include <sentry.h>
#include <string>
#include <value.hpp>
#include <vector>
#include <vendor/catch.hpp>

struct MockTransportData {
    std::vector<sentry::Value> events;
};

static MockTransportData mock_transport;

static void send_event(sentry_value_t event, void *data) {
    mock_transport.events.push_back(sentry::Value(event));
}

struct SentryGuard {
    SentryGuard(sentry_options_t *options) {
        if (!options) {
            options = sentry_options_new();
        }
        mock_transport = MockTransportData();
        sentry_options_set_transport(options, send_event, nullptr);
        sentry_init(options);
        m_done = false;
    }

    void done() {
        if (m_done) {
            return;
        }
        sentry_shutdown();
        m_done = true;
    }

    ~SentryGuard() {
        done();
    }

    bool m_done;
};

#define WITH_MOCK_TRANSPORT(Options) \
    for (SentryGuard _guard(Options); !_guard.m_done; _guard.done())

TEST_CASE("init and shutdown", "[api]") {
    for (size_t i = 0; i < 10; i++) {
        sentry_options_t *options = sentry_options_new();
        sentry_options_set_environment(options, "release");
        sentry_init(options);
        sentry_shutdown();
    }
}

static sentry_value_t dummy_before_send(sentry_value_t event,
                                        void *hint,
                                        void *closure) {
    sentry_value_t extra = sentry_value_new_object();
    sentry_value_set_by_key(extra, "foo",
                            sentry_value_new_string((const char *)closure));
    sentry_value_set_by_key(event, "extra", extra);
    return event;
}

TEST_CASE("send basic event", "[api]") {
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_environment(options, "demo-env");
    sentry_options_set_release(options, "demo-app@1.0.0");
    sentry_options_set_before_send(options, dummy_before_send,
                                   (void *)"a value");

    WITH_MOCK_TRANSPORT(options) {
        sentry_value_t event = sentry_value_new_event();
        sentry_value_t msg = sentry_value_new_string("Hello World!");
        sentry_value_set_by_key(event, "message", msg);
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);

        sentry::Value event_out = mock_transport.events[0];
        REQUIRE(event_out.get_by_key("event_id").type() ==
                SENTRY_VALUE_TYPE_STRING);
        REQUIRE(event_out.get_by_key("debug_meta").type() ==
                SENTRY_VALUE_TYPE_OBJECT);
        REQUIRE(event_out.get_by_key("sdk").type() == SENTRY_VALUE_TYPE_OBJECT);
        REQUIRE(event_out.get_by_key("timestamp").type() ==
                SENTRY_VALUE_TYPE_STRING);

        sentry::Value images = event_out.navigate("debug_meta.images");
        REQUIRE(images.type() == SENTRY_VALUE_TYPE_LIST);
        REQUIRE(images.length() > 0);

        REQUIRE(event_out.get_by_key("environment").as_cstr() ==
                std::string("demo-env"));
        REQUIRE(event_out.get_by_key("release").as_cstr() ==
                std::string("demo-app@1.0.0"));

        for (size_t i = 0; i < images.length(); i++) {
            sentry::Value image = images.get_by_index(i);
            REQUIRE(image.get_by_key("type").type() ==
                    SENTRY_VALUE_TYPE_STRING);
            REQUIRE(((image.get_by_key("debug_id").type() ==
                      SENTRY_VALUE_TYPE_STRING) ||
                     (image.get_by_key("code_id").type() ==
                      SENTRY_VALUE_TYPE_STRING)));
            REQUIRE(image.get_by_key("image_addr").type() ==
                    SENTRY_VALUE_TYPE_STRING);
        }

        sentry::Value foo_extra = event_out.navigate("extra.foo");
        REQUIRE(foo_extra.as_cstr() == std::string("a value"));
    }
}

TEST_CASE("send message event", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_value_t msg_event = sentry_value_new_message_event(
            SENTRY_LEVEL_WARNING, "root_logger", "Hello World!");
        sentry_capture_event(msg_event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.get_by_key("level").as_cstr() ==
                std::string("warning"));
        REQUIRE(event_out.get_by_key("message").as_cstr() ==
                std::string("Hello World!"));
        REQUIRE(event_out.get_by_key("logger").as_cstr() ==
                std::string("root_logger"));
    }
}

// function is non static as dladdr on linux otherwise won't give us a name.
// For this test we however want to see if we can recover a name here.
void dummy_function() {
    printf("dummy here\n");
}

TEST_CASE("send basic stacktrace", "[api][!mayfail]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_value_t msg_event = sentry_value_new_message_event(
            SENTRY_LEVEL_WARNING, nullptr, "Hello World!");
        void *addr = (void *)((char *)(void *)&dummy_function + 1);
        sentry_event_value_add_stacktrace(msg_event, &addr, 1);
        sentry_capture_event(msg_event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        sentry::Value frame =
            event_out.navigate("threads.0.stacktrace.frames.0");
        REQUIRE(frame.get_by_key("instruction_addr") ==
                sentry::Value::new_addr((uint64_t)addr));
        REQUIRE(frame.get_by_key("symbol_addr") ==
                sentry::Value::new_addr((uint64_t)(void *)&dummy_function));
        REQUIRE(frame.get_by_key("function").as_cstr() ==
                std::string("_Z14dummy_functionv"));
    }
}

TEST_CASE("send basic stacktrace (unwound)", "[api][!mayfail]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_value_t msg_event = sentry_value_new_message_event(
            SENTRY_LEVEL_WARNING, nullptr, "Hello World!");
        sentry_event_value_add_stacktrace(msg_event, nullptr, 0);
        sentry_capture_event(msg_event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        sentry::Value frames =
            event_out.navigate("threads.0.stacktrace.frames");
        REQUIRE(frames.length() > 5);

        bool found_main = false;
        for (size_t i = 0; i < frames.length(); i++) {
            sentry::Value main_frame = frames.get_by_index(i);
            if (main_frame.get_by_key("function").as_cstr() ==
                std::string("main")) {
                found_main = true;
                break;
            }
        }
        REQUIRE(found_main);

        sentry::Value api_func_frame = frames.get_by_index(frames.length() - 2);
        REQUIRE(api_func_frame.get_by_key("function").as_cstr() ==
                std::string("sentry_event_value_add_stacktrace"));
    }
}
