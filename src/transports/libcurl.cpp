#include "libcurl.hpp"
#include "../options.hpp"

using namespace sentry;
using namespace transports;

LibcurlTransport::LibcurlTransport() {
    m_worker.start();
    m_curl = curl_easy_init();
}

LibcurlTransport::~LibcurlTransport() {
    curl_easy_cleanup(m_curl);
    m_worker.kill();
}

void LibcurlTransport::shutdown() {
    m_worker.shutdown();
}

void LibcurlTransport::sendEvent(Value event) {
    m_worker.submitTask([this, event]() {
        curl_easy_reset(this->m_curl);

        const sentry_options_t *opts = sentry_get_options();
        std::string url = opts->dsn.get_store_url();
        std::string payload = event.serializeToString();

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Expect;");
        headers = curl_slist_append(headers, "Content-Type:application/x-msgpack");
        // TOOD: add auth

        curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);
        // TODO: send

        curl_slist_free_all(headers);
    });
}
