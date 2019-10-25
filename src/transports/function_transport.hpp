#ifndef SENTRY_TRANSPORTS_FUNCTION_HPP_INCLUDED
#define SENTRY_TRANSPORTS_FUNCTION_HPP_INCLUDED

#include <functional>

#include "base_transport.hpp"

namespace sentry {
namespace transports {
class FunctionTransport : public Transport {
   public:
    FunctionTransport(std::function<void(Envelope)> func) : m_func(func) {
    }
    void send_envelope(Envelope envelope);

   private:
    std::function<void(Envelope)> m_func;
};
}  // namespace transports
}  // namespace sentry

#endif
