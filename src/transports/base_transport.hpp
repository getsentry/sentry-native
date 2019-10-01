#ifndef SENTRY_TRANSPORTS_BASE_HPP_INCLUDED
#define SENTRY_TRANSPORTS_BASE_HPP_INCLUDED

#include <sstream>
#include "../internal.hpp"
#include "../path.hpp"
#include "../value.hpp"

namespace sentry {
namespace transports {

class Payload {
   public:
    Payload();
    Payload(sentry::Value event);
    Payload(const sentry::Path &path);
    Payload(const char *bytes, size_t length);

    bool is_event() const;
    sentry::Value get_event() const;
    size_t length() const;

    void serialize_into(std::ostream &out) const;

   protected:
    bool m_is_event;
    sentry::Value m_event;
    sentry::Path m_path;
    std::string m_bytes;
};

class EnvelopeItem {
   public:
    EnvelopeItem(Payload payload);

    void set_header(const char *key, sentry::Value value);
    const Payload *payload() const {
        return &m_payload;
    }

    void serialize_into(std::ostream &out) const;

   protected:
    sentry::Value m_headers;
    Payload m_payload;
};

class Envelope {
   public:
    Envelope();
    Envelope(sentry::Value event);
    sentry::Value get_event() const;

    void set_header(const char *key, sentry::Value value);
    void add_item(EnvelopeItem item);

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
