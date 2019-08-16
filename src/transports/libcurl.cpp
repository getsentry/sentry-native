#include "libcurl.hpp"
#include "../options.hpp"

using namespace sentry;
using namespace transports;

LibcurlTransport::LibcurlTransport() {
    static bool curl_initialized = false;
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    m_curl = curl_easy_init();
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

void LibcurlTransport::sendEvent(Value event) {
    const char *event_id = event.getByKey("event_id").asCStr();
    SENTRY_LOGF("Sending event %s", *event_id ? event_id : "<no client id>");
    m_worker.submitTask([this, event]() {
        const sentry_options_t *opts = sentry_get_options();
        if (opts->dsn.disabled()) {
            return;
        }

        std::string url = opts->dsn.get_store_url();
        std::string payload = event.toJson();
        std::string auth =
            std::string("X-Sentry-Auth:") + opts->dsn.get_auth_header();

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Expect:");
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "Content-Type:application/json");

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

        if (!opts->http_proxy.empty()) {
            curl_easy_setopt(this->m_curl, CURLOPT_PROXY,
                             opts->http_proxy.c_str());
        }
        if (!opts->ca_certs.empty()) {
            curl_easy_setopt(this->m_curl, CURLOPT_CAPATH,
                             opts->ca_certs.c_str());
        }

        CURLcode _rv = curl_easy_perform(this->m_curl);

        // TODO: error handling

        curl_slist_free_all(headers);
    });
}
