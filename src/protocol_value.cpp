#include "protocol_value.hpp"
#include <cmath>
#include <ctime>
#include <sstream>

using namespace sentry;

void Value::toMsgpack(mpack_writer_t *writer) const {
    switch (this->type()) {
        case SENTRY_VALUE_TYPE_NULL:
            mpack_write_nil(writer);
            break;
        case SENTRY_VALUE_TYPE_BOOL:
            mpack_write_bool(writer, this->asBool());
            break;
        case SENTRY_VALUE_TYPE_INT32:
            mpack_write_int(writer, (int64_t)this->asInt32());
            break;
        case SENTRY_VALUE_TYPE_DOUBLE:
            mpack_write_double(writer, this->asDouble());
            break;
        case SENTRY_VALUE_TYPE_STRING: {
            mpack_write_cstr_or_nil(writer, asCStr());
            break;
        }
        case SENTRY_VALUE_TYPE_LIST: {
            const List *list = (const List *)asThing()->ptr();
            mpack_start_array(writer, (uint32_t)list->size());
            for (List::const_iterator iter = list->begin(); iter != list->end();
                 ++iter) {
                iter->toMsgpack(writer);
            }
            mpack_finish_array(writer);
            break;
        }
        case SENTRY_VALUE_TYPE_OBJECT: {
            const Object *object = (const Object *)asThing()->ptr();
            mpack_start_map(writer, (uint32_t)object->size());
            for (Object::const_iterator iter = object->begin();
                 iter != object->end(); ++iter) {
                mpack_write_cstr(writer, iter->first.c_str());
                iter->second.toMsgpack(writer);
            }
            mpack_finish_map(writer);
            break;
        }
    }
}

std::string Value::toMsgpack() const {
    mpack_writer_t writer;
    char *buf;
    size_t size;
    mpack_writer_init_growable(&writer, &buf, &size);
    toMsgpack(&writer);
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
                out << "\\\b";
                break;
            case '\f':
                out << "\\\f";
                break;
            case '\n':
                out << "\\\n";
                break;
            case '\r':
                out << "\\\r";
                break;
            case '\t':
                out << "\\\t";
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

void Value::toJson(std::stringstream &out) const {
    switch (this->type()) {
        case SENTRY_VALUE_TYPE_NULL:
            out << "null";
            break;
        case SENTRY_VALUE_TYPE_BOOL:
            out << (this->asBool() ? "true" : "false");
            break;
        case SENTRY_VALUE_TYPE_INT32:
            out << this->asInt32();
            break;
        case SENTRY_VALUE_TYPE_DOUBLE: {
            double val = this->asDouble();
            if (isnan(val) || isinf(val)) {
                out << "null";
            } else {
                out << val;
            }
            break;
        }
        case SENTRY_VALUE_TYPE_STRING: {
            json_serialize_string(asCStr(), out);
            break;
        }
        case SENTRY_VALUE_TYPE_LIST: {
            const List *list = (const List *)asThing()->ptr();
            out << "[";
            for (List::const_iterator iter = list->begin(); iter != list->end();
                 ++iter) {
                if (iter != list->begin()) {
                    out << ",";
                }
                iter->toJson(out);
            }
            out << "]";
            break;
        }
        case SENTRY_VALUE_TYPE_OBJECT: {
            const Object *object = (const Object *)asThing()->ptr();
            out << "{";
            for (Object::const_iterator iter = object->begin();
                 iter != object->end(); ++iter) {
                if (iter != object->begin()) {
                    out << ",";
                }
                json_serialize_string(iter->first.c_str(), out);
                out << ":";
                iter->second.toJson(out);
            }
            out << "}";
            break;
        }
    }
}

std::string Value::toJson() const {
    std::stringstream ss;
    toJson(ss);
    return ss.str();
}

Value::~Value() {
    Thing *thing = this->asThing();
    if (thing) {
        thing->decref();
    }
}

Value Value::newEvent() {
    Value rv = Value::newObject();
    rv.setKey("level", Value::newString("error"));

    sentry_uuid_t uuid = sentry_uuid_new_v4();
    char uuid_str[40];
    sentry_uuid_as_string(&uuid, uuid_str);
    rv.setKey("event_id", Value::newString(uuid_str));

    time_t now;
    time(&now);
    char buf[255];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    rv.setKey("timestamp", Value::newString(buf));

    return rv;
}

Value Value::newBreadcrumb(const char *type, const char *message) {
    Value rv = Value::newObject();

    time_t now;
    time(&now);
    char buf[255];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    rv.setKey("timestamp", Value::newString(buf));

    if (type) {
        rv.setKey("type", Value::newString(type));
    }
    if (message) {
        rv.setKey("message", Value::newString(message));
    }

    return rv;
}

sentry_value_t sentry_value_new_null() {
    return Value::newNull().lower();
}

sentry_value_t sentry_value_new_int32(int32_t value) {
    return Value::newInt32(value).lower();
}

sentry_value_t sentry_value_new_double(double value) {
    return Value::newDouble(value).lower();
}

sentry_value_t sentry_value_new_bool(int value) {
    return Value::newBool((bool)value).lower();
}

sentry_value_t sentry_value_new_string(const char *value) {
    return Value::newString(value).lower();
}

sentry_value_t sentry_value_new_list() {
    return Value::newList().lower();
}

sentry_value_t sentry_value_new_object() {
    return Value::newObject().lower();
}

void sentry_value_incref(sentry_value_t value) {
    Value(value).incref();
}

void sentry_value_decref(sentry_value_t value) {
    Value(value).decref();
}

sentry_value_type_t sentry_value_get_type(sentry_value_t value) {
    return Value(value).type();
}

int sentry_value_set_key(sentry_value_t value,
                         const char *k,
                         sentry_value_t v) {
    return !Value(value).setKey(k, Value::consume(v));
}

int sentry_value_remove_key(sentry_value_t value, const char *k) {
    return !Value(value).removeKey(k);
}

int sentry_value_append(sentry_value_t value, sentry_value_t v) {
    return !Value(value).append(Value::consume(v));
}

sentry_value_t sentry_value_get_by_key(sentry_value_t value, const char *k) {
    return Value(value).getByKey(k).lower();
}

sentry_value_t sentry_value_get_by_index(sentry_value_t value, size_t index) {
    return Value(value).getByIndex(index).lower();
}

size_t sentry_value_get_length(sentry_value_t value) {
    return Value(value).length();
}

int32_t sentry_value_as_int32(sentry_value_t value) {
    return Value(value).asInt32();
}

double sentry_value_as_double(sentry_value_t value) {
    return Value(value).asDouble();
}

const char *sentry_value_as_string(sentry_value_t value) {
    return Value(value).asCStr();
}

int sentry_value_is_true(sentry_value_t value) {
    return Value(value).asBool();
}

int sentry_value_is_null(sentry_value_t value) {
    return Value(value).isNull();
}

sentry_value_t sentry_value_new_event(void) {
    return Value::newEvent().lower();
}

sentry_value_t sentry_value_new_breadcrumb(const char *type,
                                           const char *message) {
    return Value::newBreadcrumb(type, message).lower();
}

void sentry_event_value_add_stacktrace(sentry_value_t value, void **ips) {
    Value event = Value(value);

    Value frames = Value::newList();
    for (; *ips; ips++) {
        char buf[100];
        sprintf(buf, "0x%llx", (unsigned long long)*ips);
        Value frame = Value::newObject();
        frame.setKey("instruction_addr", Value::newString(buf));
        frames.append(frame);
    }
    frames.reverse();

    Value stacktrace = Value::newObject();
    stacktrace.setKey("frames", frames);

    Value threads = Value::newList();
    Value thread = Value::newObject();
    thread.setKey("stacktrace", stacktrace);
    threads.append(thread);

    event.setKey("threads", threads);
}
