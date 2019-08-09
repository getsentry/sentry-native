#ifndef SENTRY_PROTOCOL_VALUE_HPP_INCLUDED
#define SENTRY_PROTOCOL_VALUE_HPP_INCLUDED

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "internal.hpp"
#include "vendor/mpack.h"

namespace sentry {

class Value;
typedef std::vector<Value> List;
typedef std::map<std::string, Value> Object;

enum ThingType {
    THING_TYPE_EMPTY,
    THING_TYPE_STRING,
    THING_TYPE_LIST,
    THING_TYPE_OBJECT,
};

class Thing {
   public:
    Thing() : m_payload(nullptr), m_type(THING_TYPE_EMPTY), m_refcount(1) {
    }

    Thing(void *ptr, ThingType type)
        : m_payload(ptr), m_type(type), m_refcount(1) {
    }

    ~Thing() {
        switch (m_type) {
            case THING_TYPE_EMPTY:
                break;
            case THING_TYPE_STRING:
                delete (std::string *)m_payload;
                break;
            case THING_TYPE_LIST:
                delete (List *)m_payload;
                break;
            case THING_TYPE_OBJECT:
                delete (List *)m_payload;
                break;
        }
    }

    void incref() {
        m_refcount++;
    }

    void decref() {
        if (--m_refcount == 0) {
            delete this;
        }
    }

    ThingType type() const {
        return m_type;
    }

    sentry_value_type_t valueType() const {
        switch (m_type) {
            case THING_TYPE_EMPTY:
                return SENTRY_VALUE_TYPE_NULL;
            case THING_TYPE_LIST:
                return SENTRY_VALUE_TYPE_LIST;
            case THING_TYPE_OBJECT:
                return SENTRY_VALUE_TYPE_OBJECT;
            case THING_TYPE_STRING:
                return SENTRY_VALUE_TYPE_STRING;
            default:
                assert(!"unreachable");
        }
    }

    void *ptr() const {
        return m_payload;
    }

   private:
    Thing(const Thing &other) = delete;
    Thing &operator=(const Thing &other) = delete;

    void *m_payload;
    ThingType m_type;
    size_t m_refcount;
};

class Value {
    union {
        mutable double m_double;
        mutable uint64_t m_bits;
    };

    static const uint64_t MAX_DOUBLE = 0xfff8000000000000ULL;
    static const uint64_t TAG_INT32 = 0xfff9000000000000ULL;
    static const uint64_t TAG_CONST = 0xfffa000000000000ULL;
    static const uint64_t TAG_THING = 0xfffb000000000000ULL;

    // this leaves us with 48 bits of space behind the tags which is enough
    // to hold 48bit pointers which is enough for now.  In theory there could be
    // 52bit of pointers which will just fit into this knowing that we will
    // always have 4 byte aligned objects.
    static const uint64_t TAG_MAX = 0xffff000000000000ULL;

    Thing *asThing() const {
        if ((m_bits & TAG_THING) == TAG_THING) {
            return (Thing *)(m_bits & ~TAG_THING);
        } else {
            return nullptr;
        }
    }

    void setNullUnsafe() {
        m_bits = (uint64_t)2 | TAG_CONST;
    }

    Value(void *ptr, ThingType type)
        : m_bits((uint64_t) new Thing(ptr, type) | TAG_THING) {
    }

   public:
    Value() {
        setNullUnsafe();
    }

    Value(sentry_value_t value) {
        m_bits = value._bits;
        incref();
    }

    Value(const Value &other) {
        *this = other;
    }

    Value(Value &&other) {
        this->m_bits = other.m_bits;
        other.setNullUnsafe();
    }

    Value &operator=(const Value &other) {
        Thing *thing = asThing();
        if (thing) {
            thing->decref();
        }
        thing = other.asThing();
        if (thing) {
            thing->incref();
        }
        m_bits = other.m_bits;
        return *this;
    }

    ~Value();

    void incref() const {
        Thing *thing = asThing();
        if (thing) {
            thing->incref();
        }
    }

    void decref() const {
        Thing *thing = asThing();
        if (thing) {
            thing->decref();
        }
    }

    static Value newDouble(double val) {
        // if we are a nan value we want to become the max double value which
        // is a NAN.
        Value rv;
        if (isnan(val)) {
            rv.m_bits = MAX_DOUBLE;
        } else {
            rv.m_double = val;
        }
        return rv;
    }

    static Value newInt32(int32_t val) {
        Value rv;
        rv.m_bits = (int64_t)val | TAG_INT32;
        return rv;
    }

    static Value newBool(bool val) {
        Value rv;
        rv.m_bits = (uint64_t)(val ? 1 : 0) | TAG_CONST;
        return rv;
    }

    static Value newNull() {
        Value rv;
        rv.m_bits = (uint64_t)2 | TAG_CONST;
        return rv;
    }

    static Value newList() {
        return Value(new List(), THING_TYPE_LIST);
    }

    static Value newObject() {
        return Value(new Object(), THING_TYPE_OBJECT);
    }

    static Value newString(const char *s) {
        return s ? Value(new std::string(s), THING_TYPE_STRING) : Value();
    }

    static Value newEvent();
    static Value newBreadcrumb(const char *type, const char *message);

    sentry_value_type_t type() const {
        switch (m_bits & TAG_MAX) {
            case TAG_CONST: {
                uint64_t val = m_bits & ~TAG_CONST;
                switch (val) {
                    case 0:
                    case 1:
                        return SENTRY_VALUE_TYPE_BOOL;
                    case 2:
                        return SENTRY_VALUE_TYPE_NULL;
                    default:
                        assert(!"unreachable");
                }
            }
            case TAG_INT32:
                return SENTRY_VALUE_TYPE_INT32;
            case TAG_THING:
                return asThing()->valueType();
            default:
                return SENTRY_VALUE_TYPE_DOUBLE;
        }
    }

    double asDouble() const {
        if (m_bits <= MAX_DOUBLE) {
            return m_double;
        } else if ((m_bits & TAG_INT32) == TAG_INT32) {
            return (double)asInt32();
        } else {
            return NAN;
        }
    }

    int32_t asInt32() const {
        if ((m_bits & TAG_INT32) == TAG_INT32) {
            return (int32_t)(m_bits & ~TAG_INT32);
        } else {
            return 0;
        }
    }

    const char *asCStr() const {
        Thing *thing = asThing();
        return thing && thing->type() == THING_TYPE_STRING
                   ? ((std::string *)thing->ptr())->c_str()
                   : "";
    }

    bool asBool() const {
        if ((m_bits & TAG_CONST) == TAG_CONST) {
            uint64_t val = m_bits & ~TAG_CONST;
            if (val == 1) {
                return true;
            }
            return false;
        } else {
            return asDouble() != 0.0;
        }
    }

    bool isNull() const {
        if ((m_bits & TAG_CONST) == TAG_CONST) {
            uint64_t val = m_bits & ~TAG_CONST;
            return val == 2;
        } else {
            Thing *thing = asThing();
            return thing && thing->type() == THING_TYPE_EMPTY;
        }
    }

    bool append(Value value) {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            List *list = (List *)thing->ptr();
            list->push_back(value);
            return true;
        }
        return false;
    }

    bool reverse() {
        Thing *thing = asThing();
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

    bool setKey(const char *key, Value value) {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_OBJECT) {
            Object *obj = (Object *)thing->ptr();
            (*obj)[key] = value;
            return true;
        }
        return false;
    }

    bool removeKey(const char *key) {
        Thing *thing = asThing();
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

    Value getByKey(const char *key) const {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_OBJECT) {
            const Object *object = (const Object *)thing->ptr();
            Object::const_iterator iter = object->find(key);
            if (iter != object->end()) {
                return iter->second;
            }
        }
        return Value::newNull();
    }

    Value getByKey(const char *key) {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_OBJECT) {
            Object *object = (Object *)thing->ptr();
            Object::iterator iter = object->find(key);
            if (iter != object->end()) {
                return iter->second;
            }
        }
        return Value::newNull();
    }

    Value getByIndex(size_t index) const {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            const List *list = (const List *)thing->ptr();
            if (index < list->size()) {
                return (*list)[index];
            }
        }
        return Value::newNull();
    }

    Value getByIndex(size_t index) {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            List *list = (List *)thing->ptr();
            if (index < list->size()) {
                return (*list)[index];
            }
        }
        return Value::newNull();
    }

    size_t length() const {
        Thing *thing = asThing();
        if (thing && thing->type() == THING_TYPE_LIST) {
            return ((const List *)thing->ptr())->size();
        } else if (thing && thing->type() == THING_TYPE_OBJECT) {
            return ((const Object *)thing->ptr())->size();
        } else if (thing && thing->type() == THING_TYPE_STRING) {
            return ((const std::string *)thing->ptr())->size();
        }
        return (size_t)-1;
    }

    void serialize(mpack_writer_t *writer) const;

    sentry_value_t lower() const {
        sentry_value_t rv;
        rv._bits = m_bits;
        return rv;
    }
};

}  // namespace sentry

#endif
