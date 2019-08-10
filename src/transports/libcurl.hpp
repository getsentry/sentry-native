#ifndef SENTRY_TRANSPORTS_LIBCURL_HPP_INCLUDED
#define SENTRY_TRANSPORTS_LIBCURL_HPP_INCLUDED

#include <curl/curl.h>
#include <curl/easy.h>
#include "../worker.hpp"
#include "base.hpp"

namespace sentry {
namespace transports {
class LibcurlTransport : public Transport {
   public:
    LibcurlTransport();
    ~LibcurlTransport();
    void shutdown();
    void sendEvent(sentry::Value value);

   private:
    BackgroundWorker m_worker;
    CURL *m_curl;
};
}  // namespace transports
}  // namespace sentry

#endif
