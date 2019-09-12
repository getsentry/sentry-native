#ifdef SENTRY_WITH_LIBCURL_TRANSPORT
#include "libcurl.hpp"
#include <algorithm>
#include <cctype>
#include "../options.hpp"

using namespace sentry;
using namespace transports;

LibcurlTransport::LibcurlTransport() {
    static bool curl_initialized = false;
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    m_curl = curl_easy_init();
    m_disabled_until = std::chrono::system_clock::now();
}

LibcurlTransport::~LibcurlTransport() {
    curl_easy_cleanup(m_curl);
    m_worker.kill();
}

void LibcurlTransport::start() {
    m_worker.start();
}

void LibcurlTransport::shutdown() {
    m_worker.shutdown();
}

size_t swallow_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}

struct HeaderInfo {
    int retry_after;
};

size_t header_callback(char *buffer,
                       size_t size,
                       size_t nitems,
                       void *userdata) {
    size_t bytes = size * nitems;
    std::string header(buffer, bytes);
    size_t sep = header.find_first_of(':');
    HeaderInfo *info = (HeaderInfo *)userdata;

    if (sep != std::string::npos) {
        std::string key = header.substr(0, sep);
        std::string value = header.substr(sep + 1);
        std::transform(key.begin(), key.end(), key.begin(), tolower);
        if (key == "retry-after") {
            info->retry_after = strtol(value.c_str(), nullptr, 10);
        }
    }

    return bytes;
}

void LibcurlTransport::send_event(Value event) {
    const char *event_id = event.get_by_key("event_id").as_cstr();
    SENTRY_LOGF("Sending event %s", *event_id ? event_id : "<no client id>");
    m_worker.submit_task([this, event]() {
        const sentry_options_t *opts = sentry_get_options();
        if (opts->dsn.disabled()) {
            return;
        }

        std::string url = opts->dsn.get_store_url();
        std::string payload = event.to_json();
        std::string auth =
            std::string("x-sentry-auth:") + opts->dsn.get_auth_header();

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "expect:");
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "content-type:application/json");

        curl_easy_reset(this->m_curl);
        curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(this->m_curl, CURLOPT_POST, (long)1);
        curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE,
                         (long)payload.size());
        curl_easy_setopt(this->m_curl, CURLOPT_USERAGENT,
                         SENTRY_SDK_USER_AGENT);
        curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, swallow_data);

        HeaderInfo info = {0};
        curl_easy_setopt(this->m_curl, CURLOPT_HEADERDATA, (void *)&info);
        curl_easy_setopt(this->m_curl, CURLOPT_HEADERFUNCTION, header_callback);

        if (!opts->http_proxy.empty()) {
            curl_easy_setopt(this->m_curl, CURLOPT_PROXY,
                             opts->http_proxy.c_str());
        }
        if (!opts->ca_certs.empty()) {
            curl_easy_setopt(this->m_curl, CURLOPT_CAPATH,
                             opts->ca_certs.c_str());
        }

        CURLcode rv = curl_easy_perform(this->m_curl);

        if (rv == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(this->m_curl, CURLINFO_RESPONSE_CODE,
                              &response_code);
            if (response_code == 429) {
                m_disabled_until = std::chrono::system_clock::now() +
                                   std::chrono::seconds(info.retry_after);
            }
        }

        curl_slist_free_all(headers);
    });
}
#endif
