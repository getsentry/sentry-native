#include "../options.hpp"
#include "libcurl_transport.hpp"
#include "winhttp_transport.hpp"

#include "base_transport.hpp"

using namespace sentry;
using namespace transports;

Transport::Transport() {
}

Transport::~Transport() {
}

void Transport::start() {
}

void Transport::shutdown() {
}

void Transport::send_event(Value event) {
    send_envelope(Envelope(event));
}

Transport *transports::create_default_transport() {
#ifdef SENTRY_WITH_LIBCURL_TRANSPORT
    return new transports::LibcurlTransport();
#elif defined(SENTRY_WITH_WINHTTP_TRANSPORT)
    return new transports::WinHttpTransport();
#endif
    return nullptr;
}
