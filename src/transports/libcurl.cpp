#include "libcurl.hpp"

using namespace sentry;
using namespace transports;

LibcurlTransport::LibcurlTransport() {
    m_worker.start();
}

LibcurlTransport::~LibcurlTransport() {
    m_worker.kill();
}

void LibcurlTransport::shutdown() {
    m_worker.shutdown();
}

void LibcurlTransport::sendEvent(Value event) {
    m_worker.submitTask([event]() {
        // do something
    });
}
