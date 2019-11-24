#include <sentry.h>
#include <string>
#include <value.hpp>
#include <vector>
#include <vendor/catch.hpp>
#include "testutils.hpp"

TEST_CASE("init and shutdown", "[api]") {
    for (size_t i = 0; i < 10; i++) {
        sentry_options_t *options = sentry_options_new();
        sentry_options_set_environment(options, "release");
        sentry_init(options);
        sentry_shutdown();
    }
}

TEST_CASE("send basic event", "[api]") {
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_environment(options, "demo-env");
    sentry_options_set_release(options, "demo-app@1.0.0");
    sentry_options_set_dist(options, "android");

    WITH_MOCK_TRANSPORT(options) {
        sentry_value_t event = sentry_value_new_event();
        sentry_value_t msg = sentry_value_new_string("Hello World!");
        sentry_value_set_by_key(event, "message", msg);
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        // Assert basic event attributes
        REQUIRE(event_out.get_by_key("event_id").type() ==
                SENTRY_VALUE_TYPE_STRING);
        REQUIRE(event_out.get_by_key("debug_meta").type() ==
                SENTRY_VALUE_TYPE_OBJECT);
        REQUIRE(event_out.get_by_key("sdk").type() == SENTRY_VALUE_TYPE_OBJECT);
        REQUIRE(event_out.get_by_key("timestamp").type() ==
                SENTRY_VALUE_TYPE_STRING);

        // Assert options attributes
        REQUIRE(event_out.get_by_key("level").as_cstr() ==
                std::string("error"));
        REQUIRE(event_out.get_by_key("environment").as_cstr() ==
                std::string("demo-env"));
        REQUIRE(event_out.get_by_key("release").as_cstr() ==
                std::string("demo-app@1.0.0"));
        REQUIRE(event_out.get_by_key("dist").as_cstr() ==
                std::string("android"));
    }
}

TEST_CASE("send event with debug images", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];
        sentry::Value images = event_out.navigate("debug_meta.images");
        REQUIRE(images.type() == SENTRY_VALUE_TYPE_LIST);
        REQUIRE(images.length() > 0);

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
    }
}

static sentry_value_t dummy_before_send(sentry_value_t event,
                                        void *hint,
                                        void *data) {
    sentry_value_t extra = sentry_value_new_object();
    sentry_value_set_by_key(extra, "foo",
                            sentry_value_new_string((const char *)data));

    sentry_value_set_by_key(event, "extra", extra);
    return event;
}

TEST_CASE("send event with before_send callback", "[api]") {
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_before_send(options, dummy_before_send,
                                   (void *)"a value");

    WITH_MOCK_TRANSPORT(options) {
        sentry_add_breadcrumb(sentry_value_new_breadcrumb("default", "crumb1"));
        sentry_add_breadcrumb(sentry_value_new_breadcrumb("http", "crumb2"));

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];
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
        REQUIRE(event_out.navigate("message.formatted").as_cstr() ==
                std::string("Hello World!"));
        REQUIRE(event_out.get_by_key("logger").as_cstr() ==
                std::string("root_logger"));
    }
}

TEST_CASE("send event with breadcrumbs disabled", "[api]") {
    // Initialize explicitly without DSN
    sentry_options_t *options = sentry_options_new();
    WITH_MOCK_TRANSPORT(options) {
        sentry_add_breadcrumb(sentry_value_new_breadcrumb("default", "crumb1"));
        sentry_add_breadcrumb(sentry_value_new_breadcrumb("http", "crumb2"));

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];
        sentry::Value crumbs_out = event_out.navigate("breadcrumbs");

        REQUIRE(crumbs_out.length() == 0);
    }
}

TEST_CASE("send event with breadcrumbs", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_add_breadcrumb(sentry_value_new_breadcrumb("default", "crumb1"));
        sentry_add_breadcrumb(sentry_value_new_breadcrumb("http", "crumb2"));

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];
        sentry::Value crumbs_out = event_out.navigate("breadcrumbs");

        REQUIRE(crumbs_out.length() == 2);
        REQUIRE(crumbs_out.navigate("0.type") ==
                sentry::Value::new_string("default"));
        REQUIRE(crumbs_out.navigate("0.message") ==
                sentry::Value::new_string("crumb1"));

        REQUIRE(crumbs_out.navigate("1.type") ==
                sentry::Value::new_string("http"));
        REQUIRE(crumbs_out.navigate("1.message") ==
                sentry::Value::new_string("crumb2"));
    }
}

TEST_CASE("send event with user", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key(user, "id", sentry_value_new_int32(42));
        sentry_set_user(user);

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.navigate("user.id") == sentry::Value::new_int32(42));
    }
}

TEST_CASE("send event with tag", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_set_tag("mytag", "myvalue");

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.navigate("tags.mytag") ==
                sentry::Value::new_string("myvalue"));
    }
}

TEST_CASE("send event with extra", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_set_extra("extra1", sentry_value_new_string("foo"));
        sentry_set_extra("extra2", sentry_value_new_int32(42));

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.navigate("extra.extra1") ==
                sentry::Value::new_string("foo"));
        REQUIRE(event_out.navigate("extra.extra2") ==
                sentry::Value::new_int32(42));
    }
}

TEST_CASE("send event with custom context", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_value_t custom = sentry_value_new_object();
        sentry_value_set_by_key(custom, "foo", sentry_value_new_string("bar"));
        sentry_set_context("custom", custom);

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.navigate("contexts.custom.foo") ==
                sentry::Value::new_string("bar"));
    }
}

TEST_CASE("send event with fingerprint", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_set_fingerprint("foo", "bar", 0);

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];
        sentry::Value fp_out = event_out.navigate("fingerprint");

        REQUIRE(fp_out.length() == 2);
        REQUIRE(fp_out.navigate("0") == sentry::Value::new_string("foo"));
        REQUIRE(fp_out.navigate("1") == sentry::Value::new_string("bar"));
    }
}

TEST_CASE("send event with transaction", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_set_transaction("my_transaction");

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.navigate("transaction") ==
                sentry::Value::new_string("my_transaction"));
    }
}

TEST_CASE("send event with level", "[api]") {
    WITH_MOCK_TRANSPORT(nullptr) {
        sentry_set_level(SENTRY_LEVEL_INFO);

        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        REQUIRE(mock_transport.events.size() == 1);
        sentry::Value event_out = mock_transport.events[0];

        REQUIRE(event_out.navigate("level") ==
                sentry::Value::new_level(SENTRY_LEVEL_INFO));
    }
}

// function is non static as dladdr on linux otherwise won't give us a name.
// For this test we however want to see if we can recover a name here.
void dummy_function() {
    printf("dummy here\n");
}

TEST_CASE("send basic stacktrace", "[api]") {
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
#if !defined(__ANDROID__)
        REQUIRE(frame.get_by_key("symbol_addr") ==
                sentry::Value::new_addr((uint64_t)(void *)&dummy_function));
        REQUIRE(strstr(frame.get_by_key("function").as_cstr(),
                       "dummy_function") != nullptr);
#endif
    }
}

#ifdef _WIN32
#define OS_MAIN_FUNC "wmain"
#else
#define OS_MAIN_FUNC "main"
#endif

#ifndef __ANDROID__
TEST_CASE("send basic stacktrace (unwound)", "[api]") {
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
        bool found_add_stacktrace = false;
        for (size_t i = 0; i < frames.length(); i++) {
            sentry::Value main_frame = frames.get_by_index(i);
            const char *func = main_frame.get_by_key("function").as_cstr();
            if (func == std::string(OS_MAIN_FUNC)) {
                found_main = true;
            } else if (func ==
                       std::string("sentry_event_value_add_stacktrace")) {
                found_add_stacktrace = true;
            }
        }
        REQUIRE(found_main);
        REQUIRE(found_add_stacktrace);
    }
}
#endif
