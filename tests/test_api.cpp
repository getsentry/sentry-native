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

static sentry_options_t *init_mock_transport(sentry_options_t *options) {
    if (!options) {
        options = sentry_options_new();
    }
    mock_transport = MockTransportData();
    sentry_options_set_transport(options, send_event, nullptr);
    sentry_init(options);
    return options;
}

#define WITH_MOCK_TRANSPORT(Options)                                     \
    for (sentry_options_t *_test_options = init_mock_transport(Options); \
         _test_options; sentry_shutdown(), _test_options = nullptr)

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
            REQUIRE(image.get_by_key("debug_id").type() ==
                    SENTRY_VALUE_TYPE_STRING);
            REQUIRE(image.get_by_key("image_addr").type() ==
                    SENTRY_VALUE_TYPE_STRING);
        }
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
