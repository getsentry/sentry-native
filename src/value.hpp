#ifndef SENTRY_VALUE_HPP_INCLUDED
#define SENTRY_VALUE_HPP_INCLUDED

#include <assert.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "internal.hpp"
#include "io.hpp"
#include "uuid.hpp"
#include "vendor/mpack.h"

namespace sentry {

class JsonWriter;
class Value;
typedef std::vector<Value> List;
typedef std::map<std::string, Value> Object;

enum ThingType {
    THING_TYPE_STRING,
    THING_TYPE_LIST,
    THING_TYPE_OBJECT,
};

class Thing {
   public:
    Thing(void *ptr, ThingType type)
        : m_payload(ptr), m_type(type), m_refcount(1) {
    }

    ~Thing() {
        switch (m_type) {
            case THING_TYPE_STRING:
                delete (std::string *)m_payload;
                break;
            case THING_TYPE_LIST:
                delete (List *)m_payload;
                break;
            case THING_TYPE_OBJECT:
                delete (Object *)m_payload;
                break;
        }
    }

    void incref() {
        ++m_refcount;
    }

    void decref() {
        if (--m_refcount == 0) {
            delete this;
        }
    }

    size_t refcount() const {
        return m_refcount;
    }

    ThingType type() const {
        return m_type;
    }

    sentry_value_type_t value_type() const {
        switch (m_type) {
            case THING_TYPE_LIST:
                return SENTRY_VALUE_TYPE_LIST;
            case THING_TYPE_OBJECT:
                return SENTRY_VALUE_TYPE_OBJECT;
            case THING_TYPE_STRING:
                return SENTRY_VALUE_TYPE_STRING;
            default:
                abort();
        }
    }

    void *ptr() const {
        return m_payload;
    }

    bool operator==(const Thing &rhs) const;

    bool operator!=(const Thing &rhs) const {
        return !(*this == rhs);
    }

   private:
    Thing() = delete;
    Thing(const Thing &other) = delete;
    Thing &operator=(const Thing &other) = delete;

    void *m_payload;
    ThingType m_type;
    std::atomic_size_t m_refcount;
};

class Value {
    sentry_value_t m_repr;

    static const uint64_t MAX_DOUBLE = 0xfff8000000000000ULL;
    static const uint64_t TAG_THING = 0xfffc000000000000ULL;
    static const uint64_t TAG_INT32 = 0xfff9000000000000ULL;
    static const uint64_t TAG_CONST = 0xfffa000000000000ULL;

    Thing *as_thing() const {
        if (m_repr._bits <= MAX_DOUBLE) {
            return nullptr;
        } else if ((m_repr._bits & TAG_THING) == TAG_THING) {
            return (Thing *)((m_repr._bits << 2) & ~TAG_THING);
        } else {
            return nullptr;
        }
    }

    void set_null_unsafe() {
        m_repr._bits = ((uint64_t)2) | TAG_CONST;
    }

    Value(void *ptr, ThingType type) {
        m_repr._bits = (((uint64_t) new Thing(ptr, type)) >> 2) | TAG_THING;
    }

   public:
    Value() {
        set_null_unsafe();
    }

    Value(sentry_value_t value) {
        m_repr._bits = value._bits;
        incref();
    }

    static Value consume(sentry_value_t value) {
        Value rv;
        rv.m_repr._bits = value._bits;
        return rv;
    }

    Value(const Value &other) : Value() {
        *this = other;
    }

    Value(Value &&other) : Value() {
        *this = other;
    }

    Value &operator=(const Value &other) {
        if (this != &other) {
            decref();
            m_repr = other.m_repr;
            incref();
        }

        return *this;
    }

    Value &operator=(Value &&other) {
        if (this != &other) {
            decref();
            this->m_repr = other.m_repr;
            other.set_null_unsafe();
        }

        return *this;
    }

    ~Value() {
        decref();
    }

    void incref() const {
        Thing *thing = as_thing();
        if (thing) {
            thing->incref();
        }
    }

    void decref() const {
        Thing *thing = as_thing();
        if (thing) {
            thing->decref();
        }
    }

    size_t refcount() const {
        Thing *thing = as_thing();
        if (thing) {
            return thing->refcount();
        } else {
            return (size_t)-1;
        }
    }

    Value clone() const;

    static Value new_double(double val) {
        // if we are a nan value we want to become the max double value which
        // is a NAN.
        Value rv;
        if (std::isnan(val)) {
            rv.m_repr._bits = MAX_DOUBLE;
        } else {
            rv.m_repr._double = val;
        }
        return rv;
    }

    static Value new_int32(int32_t val) {
        Value rv;
        rv.m_repr._bits = (uint64_t)(uint32_t)val | TAG_INT32;
        return rv;
    }

    static Value new_bool(bool val) {
        Value rv;
        rv.m_repr._bits = (uint64_t)(val ? 1 : 0) | TAG_CONST;
        return rv;
    }

    static Value new_null() {
        Value rv;
        rv.m_repr._bits = (uint64_t)2 | TAG_CONST;
        return rv;
    }

    static Value new_list() {
        return Value(new List(), THING_TYPE_LIST);
    }

    static Value new_object() {
        return Value(new Object(), THING_TYPE_OBJECT);
    }

    static Value new_string(const char *s) {
        return Value(new std::string(s), THING_TYPE_STRING);
    }

    static Value new_string(const char *s, size_t len) {
        return Value(new std::string(s, len), THING_TYPE_STRING);
    }

#ifdef _WIN32
    static Value new_string(const wchar_t *s);
#endif

    static Value new_uuid(const sentry_uuid_t *uuid);
    static Value new_level(sentry_level_t level);
    static Value new_hexstring(const char *bytes, size_t len);
    static Value new_addr(uint64_t addr);
    static Value new_event();
    static Value new_breadcrumb(const char *type, const char *message);

    sentry_value_type_t type() const {
        if (m_repr._bits <= MAX_DOUBLE) {
            return SENTRY_VALUE_TYPE_DOUBLE;
        } else if ((m_repr._bits & TAG_THING) == TAG_THING) {
            return as_thing()->value_type();
        } else if ((m_repr._bits & TAG_CONST) == TAG_CONST) {
            uint64_t val = m_repr._bits & ~TAG_CONST;
            switch (val) {
                case 0:
                case 1:
                    return SENTRY_VALUE_TYPE_BOOL;
                case 2:
                    return SENTRY_VALUE_TYPE_NULL;
                default:
                    assert(!"unreachable");
            }
        } else if ((m_repr._bits & TAG_INT32) == TAG_INT32) {
            return SENTRY_VALUE_TYPE_INT32;
        }
        return SENTRY_VALUE_TYPE_DOUBLE;
    }

    double as_double() const {
        if (m_repr._bits <= MAX_DOUBLE) {
            return m_repr._double;
        } else if ((m_repr._bits & TAG_INT32) == TAG_INT32) {
            return (double)as_int32();
        } else {
            return NAN;
        }
    }

    int32_t as_int32() const {
        if ((m_repr._bits & TAG_INT32) == TAG_INT32) {
            return (int32_t)(m_repr._bits & ~TAG_INT32);
        } else {
            return 0;
        }
    }

    uint64_t as_addr() const;
    sentry_uuid_t as_uuid() const;

    const char *as_cstr() const {
        Thing *thing = as_thing();
        return thing && thing->type() == THING_TYPE_STRING
                   ? ((std::string *)thing->ptr())->c_str()
                   : "";
    }

    List *as_list() {
        Thing *thing = as_thing();
        return thing && thing->type() == THING_TYPE_LIST ? (List *)thing->ptr()
                                                         : nullptr;
    }

    const List *as_list() const {
        Thing *thing = as_thing();
        return thing && thing->type() == THING_TYPE_LIST
                   ? (const List *)thing->ptr()
                   : nullptr;
    }

    Object *as_object() {
        Thing *thing = as_thing();
        return thing && thing->type() == THING_TYPE_OBJECT
                   ? (Object *)thing->ptr()
                   : nullptr;
    }

    const Object *as_object() const {
        Thing *thing = as_thing();
        return thing && thing->type() == THING_TYPE_OBJECT
                   ? (const Object *)thing->ptr()
                   : nullptr;
    }

    bool as_bool() const {
        switch (type()) {
            case SENTRY_VALUE_TYPE_BOOL:
            case SENTRY_VALUE_TYPE_NULL: {
                uint64_t val = m_repr._bits & ~TAG_CONST;
                if (val == 1) {
                    return true;
                }
                return false;
            }
            case SENTRY_VALUE_TYPE_INT32:
            case SENTRY_VALUE_TYPE_DOUBLE:
                return as_double() != 0.0;
            default:
                return length() > 0;
        }
    }

    bool is_null() const {
        if ((m_repr._bits & TAG_CONST) == TAG_CONST) {
            uint64_t val = m_repr._bits & ~TAG_CONST;
            return val == 2;
        }
        return false;
    }

    bool append(Value value) {
        return append_bounded(value, ~0);
    }

    bool merge_key(const char *key, Value value);

    bool append_bounded(Value value, size_t maxItems) {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            List *list = (List *)thing->ptr();
            if (list->size() >= maxItems) {
                size_t overhead = list->size() - maxItems + 1;
                list->erase(list->begin(), list->begin() + overhead);
            }

            list->push_back(value);
            return true;
        }
        return false;
    }

    bool reverse() {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            List *list = (List *)thing->ptr();
            std::reverse(list->begin(), list->end());
            return true;
        } else if (thing && thing->type() == THING_TYPE_STRING) {
            std::string *str = (std::string *)thing->ptr();
            std::reverse(str->begin(), str->end());
            return true;
        }
        return false;
    }

    Value navigate(const char *path) const;

    bool set_by_key(const char *key, Value value) {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_OBJECT) {
            Object *obj = (Object *)thing->ptr();
            (*obj)[key] = value;
            return true;
        }
        return false;
    }

    bool remove_by_key(const char *key) {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_OBJECT) {
            Object *object = (Object *)thing->ptr();
            Object::iterator iter = object->find(key);
            if (iter != object->end()) {
                object->erase(iter);
                return true;
            }
        }
        return false;
    }

    bool set_by_index(size_t index, Value value) {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            List *list = (List *)thing->ptr();
            if (index >= list->size()) {
                list->resize(index + 1);
            }
            (*list)[index] = value;
            return true;
        }
        return false;
    }

    bool remove_by_index(size_t index) {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            List *list = (List *)thing->ptr();
            if (index >= list->size()) {
                return true;
            }
            list->erase(list->begin() + index);
            return true;
        }
        return false;
    }

    Value get_by_key(const char *key) const {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_OBJECT) {
            const Object *object = (const Object *)thing->ptr();
            Object::const_iterator iter = object->find(key);
            if (iter != object->end()) {
                return iter->second;
            }
        }
        return Value::new_null();
    }

    Value get_by_index(size_t index) const {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            const List *list = (const List *)thing->ptr();
            if (index < list->size()) {
                return (*list)[index];
            }
        }
        return Value::new_null();
    }

    size_t length() const {
        Thing *thing = as_thing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            return ((const List *)thing->ptr())->size();
        } else if (thing && thing->type() == THING_TYPE_OBJECT) {
            return ((const Object *)thing->ptr())->size();
        } else if (thing && thing->type() == THING_TYPE_STRING) {
            return ((const std::string *)thing->ptr())->size();
        }
        return -1;
    }

    void to_msgpack(mpack_writer_t *writer) const;
    char *to_msgpack_string(size_t *size_out) const;
    void to_json(sentry::IoWriter &out) const;
    void to_json(sentry::JsonWriter &out) const;
    char *to_json() const;

    sentry_value_t lower() {
        sentry_value_t rv;
        rv._bits = m_repr._bits;
        set_null_unsafe();
        return rv;
    }

    sentry_value_t lower_decref() {
        sentry_value_t rv;
        // if we're a thing, decref
        decref();
        rv._bits = m_repr._bits;
        set_null_unsafe();
        return rv;
    }

    bool operator==(const sentry::Value &rhs) const {
        if (type() != rhs.type()) {
            return false;
        }
        Thing *thisThing = as_thing();
        Thing *otherThing = rhs.as_thing();

        if (!thisThing) {
            if (otherThing) {
                return false;
            }
            return m_repr._bits == rhs.m_repr._bits;
        }

        return *thisThing == *otherThing;
    }

    bool operator!=(const sentry::Value &rhs) const {
        return !(*this == rhs);
    }
};  // namespace sentry

template <typename Os>
Os &operator<<(Os &os, const sentry::Value &value) {
    sentry::MemoryIoWriter writer;
    value.to_json(writer);
    os << writer.buf();
    return os;
}

}  // namespace sentry

#endif
