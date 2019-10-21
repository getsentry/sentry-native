#ifndef SENTRY_TESTS_TESTUTILS_HPP_INCLUDED
#define SENTRY_TESTS_TESTUTILS_HPP_INCLUDED

#include <sentry.h>
#include <value.hpp>
#include <vector>

struct MockTransportData {
    std::vector<sentry::Value> events;
};

extern MockTransportData mock_transport;

static void send_envelope(const sentry_envelope_t *envelope, void *data) {
    mock_transport.events.push_back(
        sentry::Value::consume(sentry_envelope_get_event(envelope)));
}

struct SentryGuard {
    SentryGuard(sentry_options_t *options) {
        if (!options) {
            options = sentry_options_new();
            sentry_options_set_dsn(options, "https://publickey@127.0.0.1/1");
        }
        mock_transport = MockTransportData();
        sentry_options_set_transport(options, send_envelope, nullptr);
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

#endif
