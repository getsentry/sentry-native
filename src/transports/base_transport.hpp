#ifndef SENTRY_TRANSPORTS_BASE_HPP_INCLUDED
#define SENTRY_TRANSPORTS_BASE_HPP_INCLUDED

#include <functional>
#include <sstream>
#include "../internal.hpp"
#include "../path.hpp"
#include "../value.hpp"

namespace sentry {
namespace transports {

enum EndpointType {
    ENDPOINT_TYPE_STORE,
    ENDPOINT_TYPE_MINIDUMP,
    ENDPOINT_TYPE_ATTACHMENT,
};

/* type of the payload envelope */
struct PreparedHttpRequest {
    std::string url;
    const char *method;
    std::vector<std::string> headers;
    std::string payload;

    PreparedHttpRequest(const sentry_uuid_t *event_id,
                        EndpointType endpoint_type,
                        const char *content_type,
                        const std::string &payload);
};

class EnvelopeItem {
   public:
    EnvelopeItem(sentry::Value event);
    EnvelopeItem(const sentry::Path &path, const char *type = "attachment");
    EnvelopeItem(const char *bytes,
                 size_t length,
                 const char *type = "attachment");

    void set_header(const char *key, sentry::Value value);

    bool is_event() const;
    bool is_minidump() const;
    bool is_attachment() const;
    const char *name() const;
    const char *filename() const;
    const char *content_type() const;
    sentry::Value get_event() const;
    size_t length() const;
    const std::string &bytes() const {
        return m_bytes;
    }

    void serialize_into(std::ostream &out) const;

   protected:
    EnvelopeItem();

    sentry::Value m_headers;
    bool m_is_event;
    sentry::Value m_event;
    sentry::Path m_path;
    std::string m_bytes;
};

class Envelope {
   public:
    Envelope();
    Envelope(sentry::Value event);
    sentry::Value get_event() const;

    void set_header(const char *key, sentry::Value value);
    sentry_uuid_t event_id() const;
    void add_item(EnvelopeItem item);
    void for_each_request(
        std::function<bool(PreparedHttpRequest &&)> func) const;

    void serialize_into(std::ostream &out) const;
    std::string serialize() const;

   protected:
    sentry::Value m_headers;
    std::vector<EnvelopeItem> m_items;
};

class Transport {
   public:
    Transport();
    virtual ~Transport();
    virtual void start();
    virtual void shutdown();
    virtual void send_event(sentry::Value event);
    virtual void send_envelope(Envelope envelope) = 0;

   private:
    Transport(const Transport &) = delete;
    Transport &operator=(Transport &) = delete;
};

Transport *create_default_transport();
}  // namespace transports
}  // namespace sentry

#endif
