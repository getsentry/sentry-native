#include "base.hpp"

#include "libcurl.hpp"
#include "winhttp.hpp"

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

Transport *transports::create_default_transport() {
#ifdef SENTRY_WITH_LIBCURL_TRANSPORT
    return new transports::LibcurlTransport();
#elif defined(SENTRY_WITH_WINHTTP_TRANSPORT)
    return new transports::WinHttpTransport();
#endif
    return nullptr;
}
