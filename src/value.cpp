#include "value.hpp"
#include <stdlib.h>
#include <cmath>
#include <codecvt>
#include <ctime>
#include <locale>
#include <sstream>

using namespace sentry;

static const char *level_as_string(sentry_level_t level) {
    switch (level) {
        case SENTRY_LEVEL_DEBUG:
            return "debug";
        case SENTRY_LEVEL_WARNING:
            return "warning";
        case SENTRY_LEVEL_ERROR:
            return "error";
        case SENTRY_LEVEL_FATAL:
            return "fatal";
        case SENTRY_LEVEL_INFO:
        default:
            return "info";
    }
}

Value Value::clone() const {
    Thing *thing = as_thing();
    if (thing) {
        Value clone;
        switch (thing->type()) {
            case THING_TYPE_LIST: {
                const List *list = (const List *)as_thing()->ptr();
                clone = Value::new_list();
                for (List::const_iterator iter = list->begin();
                     iter != list->end(); ++iter) {
                    clone.append(*iter);
                }
                break;
            }
            case THING_TYPE_OBJECT: {
                const Object *obj = (const Object *)as_thing()->ptr();
                clone = Value::new_list();
                for (Object::const_iterator iter = obj->begin();
                     iter != obj->end(); ++iter) {
                    clone.set_by_key(iter->first.c_str(), iter->second.clone());
                }
                break;
            }
            case THING_TYPE_STRING: {
                clone = *this;
            }
        }
        return clone;
    } else {
        return *this;
    }
}

void Value::to_msgpack(mpack_writer_t *writer) const {
    switch (this->type()) {
        case SENTRY_VALUE_TYPE_NULL:
            mpack_write_nil(writer);
            break;
        case SENTRY_VALUE_TYPE_BOOL:
            mpack_write_bool(writer, this->as_bool());
            break;
        case SENTRY_VALUE_TYPE_INT32:
            mpack_write_int(writer, (int64_t)this->as_int32());
            break;
        case SENTRY_VALUE_TYPE_DOUBLE:
            mpack_write_double(writer, this->as_double());
            break;
        case SENTRY_VALUE_TYPE_STRING: {
            mpack_write_cstr_or_nil(writer, as_cstr());
            break;
        }
        case SENTRY_VALUE_TYPE_LIST: {
            const List *list = (const List *)as_thing()->ptr();
            mpack_start_array(writer, (uint32_t)list->size());
            for (List::const_iterator iter = list->begin(); iter != list->end();
                 ++iter) {
                iter->to_msgpack(writer);
            }
            mpack_finish_array(writer);
            break;
        }
        case SENTRY_VALUE_TYPE_OBJECT: {
            const Object *object = (const Object *)as_thing()->ptr();
            mpack_start_map(writer, (uint32_t)object->size());
            for (Object::const_iterator iter = object->begin();
                 iter != object->end(); ++iter) {
                mpack_write_cstr(writer, iter->first.c_str());
                iter->second.to_msgpack(writer);
            }
            mpack_finish_map(writer);
            break;
        }
    }
}

std::string Value::to_msgpack() const {
    mpack_writer_t writer;
    char *buf;
    size_t size;
    mpack_writer_init_growable(&writer, &buf, &size);
    to_msgpack(&writer);
    mpack_writer_destroy(&writer);
    return std::string(buf, size);
}

namespace {
template <typename Out>
void json_serialize_string(const char *ptr, Out &out) {
    out << "\"";
    for (; *ptr; ptr++) {
        switch (*ptr) {
            case '\\':
                out << "\\";
                break;
            case '"':
                out << "\\\"";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                if (*ptr < 32) {
                    char buf[10];
                    sprintf(buf, "u%04x", *ptr);
                    out << buf;
                } else {
                    out << *ptr;
                }
        }
    }
    out << "\"";
}
}  // namespace

void Value::to_json(std::stringstream &out) const {
    switch (this->type()) {
        case SENTRY_VALUE_TYPE_NULL:
            out << "null";
            break;
        case SENTRY_VALUE_TYPE_BOOL:
            out << (this->as_bool() ? "true" : "false");
            break;
        case SENTRY_VALUE_TYPE_INT32:
            out << this->as_int32();
            break;
        case SENTRY_VALUE_TYPE_DOUBLE: {
            double val = this->as_double();
            if (std::isnan(val) || std::isinf(val)) {
                out << "null";
            } else {
                out << val;
            }
            break;
        }
        case SENTRY_VALUE_TYPE_STRING: {
            json_serialize_string(as_cstr(), out);
            break;
        }
        case SENTRY_VALUE_TYPE_LIST: {
            const List *list = (const List *)as_thing()->ptr();
            out << "[";
            for (List::const_iterator iter = list->begin(); iter != list->end();
                 ++iter) {
                if (iter != list->begin()) {
                    out << ",";
                }
                iter->to_json(out);
            }
            out << "]";
            break;
        }
        case SENTRY_VALUE_TYPE_OBJECT: {
            const Object *object = (const Object *)as_thing()->ptr();
            out << "{";
            for (Object::const_iterator iter = object->begin();
                 iter != object->end(); ++iter) {
                if (iter != object->begin()) {
                    out << ",";
                }
                json_serialize_string(iter->first.c_str(), out);
                out << ":";
                iter->second.to_json(out);
            }
            out << "}";
            break;
        }
    }
}

std::string Value::to_json() const {
    std::stringstream ss;
    to_json(ss);
    return ss.str();
}

#ifdef _WIN32
Value Value::new_string(const wchar_t *s) {
    std::string str =
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>{}.to_bytes(s);
    return Value::new_string(str.c_str());
}
#endif

Value Value::new_uuid(const sentry_uuid_t *uuid) {
    char buf[100];
    sentry_uuid_as_string(uuid, buf);
    return Value::new_string(buf);
}

Value Value::new_level(sentry_level_t level) {
    return Value::new_string(level_as_string(level));
}

Value Value::new_hexstring(const char *bytes, size_t len) {
    std::vector<char> rv(len * 2 + 1);
    char *ptr = &rv[0];
    for (size_t i = 0; i < len; i++) {
        ptr += sprintf(ptr, "%02hhx", bytes[i]);
    }
    return Value::new_string(&rv[0]);
}

Value Value::new_addr(uint64_t addr) {
    char buf[100];
    sprintf(buf, "0x%llx", (unsigned long long)addr);
    return Value::new_string(buf);
}

Value Value::new_event() {
    Value rv = Value::new_object();
    rv.set_by_key("level", Value::new_string("error"));

    sentry_uuid_t uuid = sentry_uuid_new_v4();
    char uuid_str[40];
    sentry_uuid_as_string(&uuid, uuid_str);
    rv.set_by_key("event_id", Value::new_string(uuid_str));

    time_t now;
    time(&now);
    char buf[255];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    rv.set_by_key("timestamp", Value::new_string(buf));

    return rv;
}

Value Value::new_breadcrumb(const char *type, const char *message) {
    Value rv = Value::new_object();

    time_t now;
    time(&now);
    char buf[255];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    rv.set_by_key("timestamp", Value::new_string(buf));

    if (type) {
        rv.set_by_key("type", Value::new_string(type));
    }
    if (message) {
        rv.set_by_key("message", Value::new_string(message));
    }

    return rv;
}

Value Value::navigate(const char *path) {
    size_t len = strlen(path);
    size_t ident_start = 0;
    Value rv = *this;

    for (size_t i = 0; i < len + 1; i++) {
        if (path[i] == '.' || path[i] == '\0') {
            std::string segment(path + ident_start, i - ident_start);
            char *end = nullptr;
            int idx = (int)strtol(segment.c_str(), &end, 10);
            if (end == segment.c_str() + segment.size()) {
                rv = rv.get_by_index((size_t)idx);
            } else {
                rv = rv.get_by_key(segment.c_str());
            }
            ident_start = i + 1;
        }
    }

    return rv;
}

bool Value::merge_key(const char *key, Value value) {
    if (value.is_null()) {
        return true;
    }

    Value existing = get_by_key(key);

    switch (value.type()) {
        case SENTRY_VALUE_TYPE_LIST: {
            if (existing.is_null()) {
                existing = Value::new_list();
            } else if (existing.type() != SENTRY_VALUE_TYPE_LIST) {
                return false;
            }
            List *dst_list = (List *)existing.as_thing()->ptr();
            List *src_list = (List *)value.as_thing()->ptr();
            dst_list->insert(dst_list->end(), src_list->begin(),
                             src_list->end());
            break;
        }
        case SENTRY_VALUE_TYPE_OBJECT: {
            if (existing.is_null()) {
                existing = Value::new_object();
            } else if (existing.type() != SENTRY_VALUE_TYPE_OBJECT) {
                return false;
            }
            Object *dst_obj = (Object *)existing.as_thing()->ptr();
            Object *src_obj = (Object *)value.as_thing()->ptr();
            dst_obj->insert(src_obj->begin(), src_obj->end());
            break;
        }
        default:
            return false;
    }

    return true;
}

sentry_value_t sentry_value_new_null() {
    return Value::new_null().lower();
}

sentry_value_t sentry_value_new_int32(int32_t value) {
    return Value::new_int32(value).lower();
}

sentry_value_t sentry_value_new_double(double value) {
    return Value::new_double(value).lower();
}

sentry_value_t sentry_value_new_bool(int value) {
    return Value::new_bool((bool)value).lower();
}

sentry_value_t sentry_value_new_string(const char *value) {
    return Value::new_string(value).lower();
}

sentry_value_t sentry_value_new_list() {
    return Value::new_list().lower();
}

sentry_value_t sentry_value_new_object() {
    return Value::new_object().lower();
}

void sentry_value_free(sentry_value_t value) {
    Value::consume(value);
}

sentry_value_type_t sentry_value_get_type(sentry_value_t value) {
    return Value(value).type();
}

int sentry_value_set_by_key(sentry_value_t value,
                            const char *k,
                            sentry_value_t v) {
    return !Value(value).set_by_key(k, Value::consume(v));
}

int sentry_value_remove_by_key(sentry_value_t value, const char *k) {
    return !Value(value).remove_by_key(k);
}

int sentry_value_append(sentry_value_t value, sentry_value_t v) {
    return !Value(value).append(Value::consume(v));
}

int sentry_value_set_by_index(sentry_value_t value,
                              size_t index,
                              sentry_value_t v) {
    return !Value(value).set_by_index(index, Value::consume(v));
}

int sentry_value_remove_by_index(sentry_value_t value, size_t index) {
    return !Value(value).remove_by_index(index);
}

sentry_value_t sentry_value_get_by_key(sentry_value_t value, const char *k) {
    return Value(value).get_by_key(k).lower();
}

sentry_value_t sentry_value_get_by_index(sentry_value_t value, size_t index) {
    return Value(value).get_by_index(index).lower();
}

size_t sentry_value_get_length(sentry_value_t value) {
    return Value(value).length();
}

int32_t sentry_value_as_int32(sentry_value_t value) {
    return Value(value).as_int32();
}

double sentry_value_as_double(sentry_value_t value) {
    return Value(value).as_double();
}

const char *sentry_value_as_string(sentry_value_t value) {
    return Value(value).as_cstr();
}

int sentry_value_is_true(sentry_value_t value) {
    return Value(value).as_bool();
}

int sentry_value_is_null(sentry_value_t value) {
    return Value(value).is_null();
}

sentry_value_t sentry_value_new_event(void) {
    return Value::new_event().lower();
}

sentry_value_t sentry_value_new_message_event(sentry_level_t level,
                                              const char *logger,
                                              const char *text) {
    Value event = Value::new_event();
    event.set_by_key("level", Value::new_level(level));
    if (logger) {
        event.set_by_key("logger", Value::new_string(logger));
    }
    if (text) {
        event.set_by_key("message", Value::new_string(text));
    }
    return event.lower();
}

sentry_value_t sentry_value_new_breadcrumb(const char *type,
                                           const char *message) {
    return Value::new_breadcrumb(type, message).lower();
}

char *sentry_value_to_json(sentry_value_t value) {
    std::string out = Value(value).to_json();
    char *rv = (char *)malloc(out.length() + 1);
    memcpy(rv, out.c_str(), out.length() + 1);
    return rv;
}

char *sentry_value_to_msgpack(sentry_value_t value, size_t *size_out) {
    std::string out = Value(value).to_msgpack();
    char *rv = (char *)malloc(out.length());
    memcpy(rv, out.c_str(), out.length());
    if (size_out) {
        *size_out = (size_t)out.length();
    }
    return rv;
}

void sentry_event_value_add_stacktrace(sentry_value_t value, void **ips) {
    Value event = Value(value);

    Value frames = Value::new_list();
    for (; *ips; ips++) {
        char buf[100];
        sprintf(buf, "0x%llx", (unsigned long long)*ips);
        Value frame = Value::new_object();
        frame.set_by_key("instruction_addr", Value::new_string(buf));
        frames.append(frame);
    }
    frames.reverse();

    Value stacktrace = Value::new_object();
    stacktrace.set_by_key("frames", frames);

    Value threads = Value::new_list();
    Value thread = Value::new_object();
    thread.set_by_key("stacktrace", stacktrace);
    threads.append(thread);

    event.set_by_key("threads", threads);
}
