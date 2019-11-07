#ifndef SENTRY_JSON_HPP_INCLUDED
#define SENTRY_JSON_HPP_INCLUDED

#include "internal.hpp"
#include "io.hpp"

namespace sentry {

// a json writer that can write into an IoWriter without allocations.  This is
// important because we want to use this thing in async safe code.
class JsonWriter {
   public:
    JsonWriter(IoWriter &writer)
        : m_writer(writer), m_want_comma(0), m_depth(0), m_last_was_key(false) {
    }

    void write_null() {
        if (can_write_item()) {
            m_writer.write_str("null");
        }
    }

    void write_bool(bool val) {
        if (can_write_item()) {
            m_writer.write_str(val ? "true" : "false");
        }
    }

    void write_int32(int32_t val) {
        if (can_write_item()) {
            m_writer.write_int32(val);
        }
    }

    void write_double(double val) {
        if (!can_write_item()) {
            return;
        }
        if (std::isnan(val) || std::isinf(val)) {
            m_writer.write_str("null");
        } else {
            m_writer.write_double(val);
        }
    }

    void write_str(const char *s) {
        if (can_write_item()) {
            do_write_string(s);
        }
    }

    void write_key(const char *s) {
        if (can_write_item()) {
            do_write_string(s);
            m_writer.write_char(':');
            m_last_was_key = true;
        }
    }

    void write_list_start() {
        if (!can_write_item()) {
            return;
        }
        m_writer.write_char('[');
        m_depth += 1;
        set_comma(false);
    }

    void write_list_end() {
        m_writer.write_char(']');
        m_depth -= 1;
    }

    void write_object_start() {
        if (!can_write_item()) {
            return;
        }
        m_writer.write_char('{');
        m_depth += 1;
        set_comma(false);
    }

    void write_object_end() {
        m_writer.write_char('}');
        m_depth -= 1;
    }

   private:
    void do_write_string(const char *ptr) {
        m_writer.write_char('"');
        for (; *ptr; ptr++) {
            switch (*ptr) {
                case '\\':
                    m_writer.write_str("\\\\");
                    break;
                case '"':
                    m_writer.write_str("\\\"");
                    break;
                case '\b':
                    m_writer.write_str("\\b");
                    break;
                case '\f':
                    m_writer.write_str("\\f");
                    break;
                case '\n':
                    m_writer.write_str("\\n");
                    break;
                case '\r':
                    m_writer.write_str("\\r");
                    break;
                case '\t':
                    m_writer.write_str("\\t");
                    break;
                default:
                    if (*ptr < 32) {
                        char buf[10];
                        sprintf(buf, "u%04x", *ptr);
                        m_writer.write_str(buf);
                    } else {
                        m_writer.write_char(*ptr);
                    }
            }
        }
        m_writer.write_char('"');
    }
    bool at_max_depth() {
        return m_depth >= 64;
    }
    bool can_write_item() {
        if (at_max_depth()) {
            return false;
        }
        if (m_last_was_key) {
            m_last_was_key = false;
            return true;
        }
        if ((m_want_comma >> m_depth) & 1) {
            m_writer.write_char(',');
        } else {
            set_comma(true);
        }
        return true;
    }
    void set_comma(bool val) {
        if (at_max_depth()) {
            return;
        }
        if (val) {
            m_want_comma |= 1ULL << m_depth;
        } else {
            m_want_comma &= (1ULL << m_depth);
        }
    }

    IoWriter &m_writer;
    uint64_t m_want_comma;
    uint32_t m_depth;
    bool m_last_was_key;
};
}  // namespace sentry

#endif
