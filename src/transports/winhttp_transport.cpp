#ifdef SENTRY_WITH_WINHTTP_TRANSPORT
#include <codecvt>
#include <iostream>
#include <locale>

#include "../options.hpp"

#include "winhttp_transport.hpp"

using namespace sentry;
using namespace transports;

WinHttpTransport::WinHttpTransport()
    : m_session(0), m_connect(0), m_disabled_until(0) {
}

WinHttpTransport::~WinHttpTransport() {
    m_worker.kill();
    if (m_connect) {
        WinHttpCloseHandle(m_connect);
    }
    if (m_session) {
        WinHttpCloseHandle(m_session);
    }
}

void WinHttpTransport::start() {
    m_worker.start();
}

void WinHttpTransport::shutdown() {
    m_worker.shutdown();
    if (m_connect) {
        WinHttpCloseHandle(m_connect);
        m_connect = 0;
    }
    if (m_session) {
        WinHttpCloseHandle(m_session);
        m_session = 0;
    }
}

static void parse_http_proxy(const char *proxy, std::wstring *proxy_out) {
    if (strstr(proxy, "http://") == 0) {
        const char *ptr = proxy + 7;
        while (*ptr && *ptr != '/') {
            proxy_out->push_back(*ptr++);
        }
    }
}

void WinHttpTransport::send_envelope(Envelope envelope) {
    this->m_worker.submit_task([this, envelope]() {
        envelope.for_each_request([this](PreparedHttpRequest prepared_request) {
            const sentry_options_t *opts = sentry_get_options();
            if (opts->dsn.disabled()) {
                return false;
            }

            ULONGLONG now = GetTickCount64();
            if (now < m_disabled_until) {
                return false;
            }

            if (!m_session) {
                std::wstring user_agent;
                const char *ptr = SENTRY_SDK_USER_AGENT;
                while (*ptr) {
                    user_agent.push_back(*ptr++);
                }

                std::wstring proxy;
                if (!opts->http_proxy.empty()) {
                    parse_http_proxy(opts->http_proxy.c_str(), &proxy);
                }
                if (!proxy.empty()) {
                    m_session = WinHttpOpen(
                        user_agent.c_str(), WINHTTP_ACCESS_TYPE_NAMED_PROXY,
                        proxy.c_str(), WINHTTP_NO_PROXY_BYPASS, 0);
                } else {
                    m_session = WinHttpOpen(
                        user_agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                }
            }

            std::wstring store_url =
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>{}
                    .from_bytes(opts->dsn.get_store_url());

            URL_COMPONENTS url_components;
            wchar_t hostname[128];
            wchar_t url_path[4096];
            memset(&url_components, 0, sizeof(URL_COMPONENTS));
            url_components.dwStructSize = sizeof(URL_COMPONENTS);
            url_components.lpszHostName = hostname;
            url_components.dwHostNameLength = 128;
            url_components.lpszUrlPath = url_path;
            url_components.dwUrlPathLength = 1024;

            WinHttpCrackUrl(store_url.c_str(), 0, 0, &url_components);
            if (!m_connect) {
                m_connect = WinHttpConnect(
                    m_session,
                    std::wstring(url_components.lpszHostName,
                                 url_components.lpszHostName +
                                     url_components.dwHostNameLength)
                        .c_str(),
                    url_components.nPort, 0);
            }

            HINTERNET request = WinHttpOpenRequest(
                m_connect, L"POST", url_components.lpszUrlPath, nullptr,
                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                opts->dsn.is_secure() ? WINHTTP_FLAG_SECURE : 0);

            std::wstringstream h;
            for (auto iter = prepared_request.headers.begin();
                 iter != prepared_request.headers.end(); ++iter) {
                h << iter->c_str() << "\r\n";
            }
            std::wstring headers = h.str();

            if (WinHttpSendRequest(request, headers.c_str(), headers.size(),
                                   (LPVOID)prepared_request.payload.c_str(),
                                   prepared_request.payload.size(),
                                   prepared_request.payload.size(), 0)) {
                DWORD status_code = 0;
                DWORD status_code_size = sizeof(DWORD);

                WinHttpQueryHeaders(
                    request,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &status_code,
                    &status_code_size, WINHTTP_NO_HEADER_INDEX);

                if (status_code == 429) {
                    DWORD retry_after = 0;
                    DWORD retry_after_size = sizeof(DWORD);
                    if (WinHttpQueryHeaders(request,
                                            WINHTTP_QUERY_RETRY_AFTER |
                                                WINHTTP_QUERY_FLAG_NUMBER,
                                            WINHTTP_HEADER_NAME_BY_INDEX,
                                            &retry_after, &retry_after_size,
                                            WINHTTP_NO_HEADER_INDEX)) {
                        m_disabled_until =
                            GetTickCount64() + retry_after * 1000;
                    }
                }

                WinHttpReceiveResponse(request, nullptr);
            }
            WinHttpCloseHandle(request);
            return true;
        });
    });
}
#endif
