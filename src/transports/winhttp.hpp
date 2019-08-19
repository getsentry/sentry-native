#ifndef SENTRY_TRANSPORTS_WINHTTP_HPP_INCLUDED
#define SENTRY_TRANSPORTS_WINHTTP_HPP_INCLUDED
#ifdef SENTRY_WITH_WINHTTP_TRANSPORT

#include <Windows.h>
#include <WinHttp.h>
#include "../worker.hpp"
#include "base.hpp"

namespace sentry {
namespace transports {
class WinHttpTransport : public Transport {
   public:
    WinHttpTransport();
    ~WinHttpTransport();
    void start();
    void shutdown();
    void send_event(sentry::Value value);

   private:
    BackgroundWorker m_worker;
    ULONGLONG m_disabled_until;
    HINTERNET m_session;
    HINTERNET m_connect;
};
}  // namespace transports
}  // namespace sentry

#endif
#endif
