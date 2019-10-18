#include "function_transport.hpp"

using namespace sentry;
using namespace transports;

void FunctionTransport::send_event(Value event) {
    m_func(event);
}

void FunctionTransport::send_envelope(Envelope envelope) {
    sentry::Value event = envelope.get_event();
    if (!event.is_null()) {
        send_event(event);
    } else {
        SENTRY_LOG("This transport cannot send these payloads. Dropped");
    }
}
