#ifndef SENTRY_TRANSPORTS_BASE_HPP_INCLUDED
#define SENTRY_TRANSPORTS_BASE_HPP_INCLUDED

#include "../internal.hpp"
#include "../value.hpp"

namespace sentry {
namespace transports {
class Transport {
   public:
    Transport();
    virtual ~Transport();
    virtual void start();
    virtual void shutdown();
    virtual void sendEvent(sentry::Value value) = 0;

   private:
    Transport(const Transport &) = delete;
    Transport &operator=(Transport &) = delete;
};
}  // namespace transports
}  // namespace sentry

#endif
