#include "function.hpp"

using namespace sentry;
using namespace transports;

void FunctionTransport::sendEvent(Value event) {
    m_func(event);
}
