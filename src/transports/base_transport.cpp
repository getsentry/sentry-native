#include "base_transport.hpp"

#include "../options.hpp"
#include "libcurl_transport.hpp"
#include "winhttp_transport.hpp"

using namespace sentry;
using namespace transports;

EnvelopeItem::EnvelopeItem()
    : m_is_event(false), m_headers(Value::new_object()) {
}

EnvelopeItem::EnvelopeItem(Value event) : EnvelopeItem() {
    m_is_event = true;
    m_event = event;
    m_bytes = m_event.to_json();
    m_headers.set_by_key("length", Value::new_int32((int32_t)m_bytes.size()));
    m_headers.set_by_key("type", Value::new_string("event"));
}

EnvelopeItem::EnvelopeItem(const sentry::Path &path, const char *type)
    : EnvelopeItem() {
    std::string rv;
    m_bytes.clear();
    m_path = path;
    FILE *f = path.open("rb");
    char buf[4096];
    if (!f) {
        return;
    }

    while (true) {
        size_t read = fread(buf, 1, sizeof(buf), f);
        if (!read) {
            break;
        }
        m_bytes.append(buf, read);
    }

    fclose(f);

    m_headers.set_by_key("length", Value::new_int32((int32_t)m_bytes.size()));
    m_headers.set_by_key("type", Value::new_string(type));
}

EnvelopeItem::EnvelopeItem(const char *bytes, size_t length, const char *type)
    : EnvelopeItem() {
    m_bytes = std::string(bytes, length);
    m_headers.set_by_key("length", Value::new_int32((int32_t)m_bytes.size()));
    m_headers.set_by_key("type", Value::new_string(type));
}

void EnvelopeItem::serialize_into(std::ostream &out) const {
    m_headers.to_json(out);
    out << "\n";
    out << m_bytes;
    out << "\n";
}

void EnvelopeItem::set_header(const char *key, sentry::Value value) {
    m_headers.set_by_key(key, value);
}

size_t EnvelopeItem::length() const {
    return m_bytes.size();
}

bool EnvelopeItem::is_event() const {
    return m_is_event;
}

Value EnvelopeItem::get_event() const {
    return m_event;
}

Envelope::Envelope() : m_headers(Value::new_object()) {
}

Envelope::Envelope(Value event) : Envelope() {
    add_item(EnvelopeItem(event));
}

void Envelope::set_header(const char *key, sentry::Value value) {
    m_headers.set_by_key(key, value);
}

void Envelope::add_item(EnvelopeItem item) {
    m_items.push_back(item);
}

void Envelope::serialize_into(std::ostream &out) const {
    m_headers.to_json(out);
    out << "\n";
    for (auto iter = m_items.begin(); iter != m_items.end(); ++iter) {
        iter->serialize_into(out);
    }
}

sentry::Value Envelope::get_event() const {
    for (auto iter = m_items.begin(); iter != m_items.end(); ++iter) {
        if (iter->is_event()) {
            return iter->get_event();
        }
    }
    return sentry::Value();
}

enum EndpointType {
    ENDPOINT_TYPE_STORE,
    ENDPOINT_TYPE_MINIDUMP,
};

static sentry_prepared_http_request_t *new_request(EndpointType endpoint_type,
                                                   const char *content_type,
                                                   size_t payload_len) {
    const sentry_options_t *options = sentry_get_options();
    sentry_prepared_http_request_t *req = new sentry_prepared_http_request_t();

    char **headers = new char *[4];
    std::string header;

    header = std::string("x-sentry-auth:") + options->dsn.get_auth_header();
    headers[0] = strdup(header.c_str());
    header = std::string("content-type:") + content_type;
    headers[1] = strdup(header.c_str());
    header = std::string("content-length:") + std::to_string(payload_len);
    headers[2] = strdup(header.c_str());
    headers[3] = nullptr;

    req->headers = headers;
    req->method = "POST";
    switch (endpoint_type) {
        case ENDPOINT_TYPE_STORE:
            req->url = strdup(options->dsn.get_store_url());
            break;
        case ENDPOINT_TYPE_MINIDUMP:
            req->url = strdup(options->dsn.get_minidump_url());
            break;
    }
    req->payload_len = payload_len;

    return req;
}

void Envelope::for_each_request(
    std::function<bool(sentry_prepared_http_request_t *)> func) {
    for (auto iter = m_items.begin(); iter != m_items.end(); ++iter) {
        if (iter->is_event()) {
            sentry_prepared_http_request_t *req = new_request(
                ENDPOINT_TYPE_STORE, "application/json", iter->length());
            req->payload = iter->clone_raw_payload();
            func(req);
        }
    }
}

std::string Envelope::serialize() const {
    std::stringstream rv;
    serialize_into(rv);
    return rv.str();
}

Transport::Transport() {
}

Transport::~Transport() {
}

void Transport::start() {
}

void Transport::shutdown() {
}

void Transport::send_envelope(Envelope envelope) {
    sentry::Value event = envelope.get_event();
    if (!event.is_null()) {
        send_event(event);
    } else {
        SENTRY_LOG("This transport cannot send these payloads. Dropped");
    }
}

Transport *transports::create_default_transport() {
#ifdef SENTRY_WITH_LIBCURL_TRANSPORT
    return new transports::LibcurlTransport();
#elif defined(SENTRY_WITH_WINHTTP_TRANSPORT)
    return new transports::WinHttpTransport();
#endif
    return nullptr;
}

void sentry_prepared_http_request_free(sentry_prepared_http_request_t *req) {
    if (!req) {
        return;
    }
    free(req->url);
    for (char *header = req->headers[0]; header; ++header) {
        free(header);
    }
    delete[] req->headers;
    free(req->payload);
    delete req;
}
