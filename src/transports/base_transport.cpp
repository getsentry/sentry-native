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

bool EnvelopeItem::is_minidump() const {
    return strcmp(m_headers.get_by_key("type").as_cstr(), "minidump") == 0;
}

bool EnvelopeItem::is_attachment() const {
    return strcmp(m_headers.get_by_key("type").as_cstr(), "attachment") == 0;
}

const char *EnvelopeItem::name() const {
    Value name = m_headers.get_by_key("name");
    if (name.is_null() && is_minidump()) {
        return "uploaded_file_minidump";
    } else {
        return name.as_cstr();
    }
}

const char *EnvelopeItem::filename() const {
    Value filename = m_headers.get_by_key("filename");
    if (filename.is_null()) {
        if (is_minidump()) {
            return "minidump.dmp";
        } else {
            return "attachment.bin";
        }
    } else {
        return filename.as_cstr();
    }
}

const char *EnvelopeItem::content_type() const {
    Value content_type = m_headers.get_by_key("content_type");
    if (content_type.is_null()) {
        if (is_minidump()) {
            return "application/x-minidump";
        } else {
            return "application/octet-stream";
        }
    } else {
        return content_type.as_cstr();
    }
}

Envelope::Envelope() : m_headers(Value::new_object()) {
}

Envelope::Envelope(Value event) : Envelope() {
    // envelopes require an event_id.
    if (event.get_by_key("event_id").is_null()) {
        sentry_uuid_t event_id = sentry_uuid_new_v4();
        event.set_by_key("event_id", Value::new_uuid(&event_id));
    }
    add_item(EnvelopeItem(event));
}

void Envelope::set_header(const char *key, sentry::Value value) {
    m_headers.set_by_key(key, value);
}

sentry_uuid_t Envelope::event_id() const {
    const char *event_id_str = m_headers.get_by_key("event_id").as_cstr();
    return sentry_uuid_from_string(event_id_str);
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

Value Envelope::get_event() const {
    for (auto iter = m_items.begin(); iter != m_items.end(); ++iter) {
        if (iter->is_event()) {
            return iter->get_event();
        }
    }
    return sentry::Value();
}

PreparedHttpRequest::PreparedHttpRequest(const sentry_uuid_t *event_id,
                                         EndpointType endpoint_type,
                                         const char *content_type,
                                         const std::string &payload)
    : method("POST"), payload(payload) {
    const sentry_options_t *options = sentry_get_options();

    headers.push_back(std::string("x-sentry-auth:") +
                      options->dsn.get_auth_header());
    headers.push_back(std::string("content-type:") + content_type);
    headers.push_back(std::string("content-length:") +
                      std::to_string(payload.size()));

    switch (endpoint_type) {
        case ENDPOINT_TYPE_STORE:
            url = options->dsn.get_store_url();
            break;
        case ENDPOINT_TYPE_MINIDUMP:
            url = options->dsn.get_minidump_url();
            break;
        case ENDPOINT_TYPE_ATTACHMENT: {
            url = options->dsn.get_attachment_url(event_id);
            break;
        }
    }
}

void Envelope::for_each_request(
    std::function<bool(PreparedHttpRequest &&)> func) {
    // this is super inefficient
    sentry_uuid_t event_id = this->event_id();
    std::vector<const EnvelopeItem *> attachments;
    const EnvelopeItem *minidump = nullptr;

    for (auto iter = m_items.begin(); iter != m_items.end(); ++iter) {
        if (iter->is_event()) {
            func(std::move(PreparedHttpRequest(&event_id, ENDPOINT_TYPE_STORE,
                                               "application/json",
                                               iter->bytes())));
        } else if (iter->is_attachment()) {
            attachments.push_back(&*iter);
        } else if (iter->is_minidump()) {
            minidump = &*iter;
        }
    }

    if (!minidump && attachments.empty()) {
        return;
    }

    EndpointType endpoint_type = ENDPOINT_TYPE_ATTACHMENT;
    if (minidump != nullptr) {
        attachments.push_back(minidump);
        endpoint_type = ENDPOINT_TYPE_MINIDUMP;
    }

    char boundary[50];
    sentry_uuid_t boundary_id = sentry_uuid_new_v4();
    sentry_uuid_as_string(&boundary_id, boundary);
    strcat(boundary, "-boundary-");

    std::stringstream payload_ss;

    for (auto iter = attachments.begin(); iter != attachments.end(); ++iter) {
        payload_ss << "--" << boundary << "\r\n";
        payload_ss << "content-type:" << (**iter).content_type() << "\r\n";
        payload_ss << "content-disposition:form-data;name=\"" << (**iter).name()
                   << "\";filename=\"" << (**iter).filename() << "\"\r\n\r\n";
        payload_ss << (**iter).bytes() << "\r\n";
    }
    payload_ss << "--" << boundary << "--";

    std::stringstream content_type_ss;
    content_type_ss << "multipart/form-data;boundary=\"" << boundary << "\"";
    std::string content_type = content_type_ss.str();
    std::string payload = payload_ss.str();

    func(std::move(PreparedHttpRequest(&event_id, ENDPOINT_TYPE_STORE,
                                       content_type.c_str(), payload)));
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
