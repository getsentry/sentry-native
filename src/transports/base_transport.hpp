#ifndef SENTRY_TRANSPORTS_BASE_HPP_INCLUDED
#define SENTRY_TRANSPORTS_BASE_HPP_INCLUDED

#include <functional>
#include <sstream>
#include "../internal.hpp"
#include "../path.hpp"
#include "../value.hpp"

namespace sentry {
namespace transports {

class EnvelopeItem {
   public:
    EnvelopeItem(sentry::Value event);
    EnvelopeItem(const sentry::Path &path, const char *type = "attachment");
    EnvelopeItem(const char *bytes,
                 size_t length,
                 const char *type = "attachment");

    void set_header(const char *key, sentry::Value value);
    char *clone_raw_payload() {
        char *rv = (char *)malloc(m_bytes.length());
        memcpy(rv, m_bytes.c_str(), m_bytes.length());
        return rv;
    }

    bool is_event() const;
    sentry::Value get_event() const;
    size_t length() const;

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
    void add_item(EnvelopeItem item);
    void for_each_request(
        std::function<bool(sentry_prepared_http_request_t *)> func);

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
    virtual void send_event(sentry::Value event) = 0;
    virtual void send_envelope(Envelope envelope);

   private:
    Transport(const Transport &) = delete;
    Transport &operator=(Transport &) = delete;
};

Transport *create_default_transport();
}  // namespace transports
}  // namespace sentry

#endif
