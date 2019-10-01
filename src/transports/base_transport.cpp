#include "base_transport.hpp"

#include "libcurl_transport.hpp"
#include "winhttp_transport.hpp"

using namespace sentry;
using namespace transports;

Payload::Payload() : m_is_event(false) {
}

Payload::Payload(Value event) : Payload() {
    m_is_event = true;
    m_event = event;
    m_bytes = m_event.to_json();
}

Payload::Payload(const sentry::Path &path) : Payload() {
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
}

Payload::Payload(const char *bytes, size_t length) : Payload() {
    m_bytes = std::string(bytes, length);
}

size_t Payload::length() const {
    return m_bytes.size();
}

bool Payload::is_event() const {
    return m_is_event;
}

Value Payload::get_event() const {
    return m_event;
}

void Payload::serialize_into(std::ostream &out) const {
    out << m_bytes;
}

EnvelopeItem::EnvelopeItem(Payload payload) : m_headers(Value::new_object()) {
    m_headers.set_by_key("length", Value::new_int32((int32_t)payload.length()));
    if (payload.is_event()) {
        m_headers.set_by_key("type", Value::new_string("event"));
    } else {
        m_headers.set_by_key("type", Value::new_string("attachment"));
    }
}

void EnvelopeItem::serialize_into(std::ostream &out) const {
    m_headers.to_json(out);
    out << "\n";
    m_payload.serialize_into(out);
    out << "\n";
}

void EnvelopeItem::set_header(const char *key, sentry::Value value) {
    m_headers.set_by_key(key, value);
}

Envelope::Envelope() : m_headers(Value::new_object()) {
}

Envelope::Envelope(Value event) : Envelope() {
    add_item(EnvelopeItem(Payload(event)));
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
        if (iter->payload()->is_event()) {
            return iter->payload()->get_event();
        }
    }
    return sentry::Value();
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
