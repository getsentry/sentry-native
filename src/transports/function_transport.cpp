#include "function_transport.hpp"

using namespace sentry;
using namespace transports;

void FunctionTransport::send_event(Value event) {
    m_func(event);
}
