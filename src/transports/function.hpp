#ifndef SENTRY_TRANSPORTS_FUNCTION_HPP_INCLUDED
#define SENTRY_TRANSPORTS_FUNCTION_HPP_INCLUDED

#include "base.hpp"
#include <functional>

namespace sentry {
namespace transports {
class FunctionTransport : public Transport {
   public:
    FunctionTransport(std::function<void(sentry::Value)> func) : m_func(func) {
    }
    void send_event(sentry::Value value);

   private:
    std::function<void(sentry::Value)> m_func;
};
}  // namespace transports
}  // namespace sentry

#endif
