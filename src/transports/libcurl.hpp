#ifndef SENTRY_TRANSPORTS_LIBCURL_HPP_INCLUDED
#define SENTRY_TRANSPORTS_LIBCURL_HPP_INCLUDED
#ifdef SENTRY_WITH_LIBCURL_TRANSPORT

#include <curl/curl.h>
#include <curl/easy.h>
#include <chrono>
#include "../worker.hpp"
#include "base.hpp"

namespace sentry {
namespace transports {
class LibcurlTransport : public Transport {
   public:
    LibcurlTransport();
    ~LibcurlTransport();
    void start();
    void shutdown();
    void send_event(sentry::Value value);

   private:
    BackgroundWorker m_worker;
    CURL *m_curl;
    std::chrono::system_clock::time_point m_disabled_until;
};
}  // namespace transports
}  // namespace sentry

#endif
#endif
