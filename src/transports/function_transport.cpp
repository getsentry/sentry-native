#include "function_transport.hpp"

using namespace sentry;
using namespace transports;

void FunctionTransport::send_envelope(Envelope envelope) {
    m_func(envelope);
}
