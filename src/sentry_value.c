#include <assert.h>
#include <sentry.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "sentry_alloc.h"
#include "sentry_json.h"
#include "sentry_string.h"
#include "sentry_utils.h"
#include "sentry_value.h"

static const uint64_t MAX_DOUBLE = 0xfff8000000000000ULL;
static const uint64_t TAG_THING = 0xfffc000000000000ULL;
static const uint64_t TAG_INT32 = 0xfff9000000000000ULL;
static const uint64_t TAG_CONST = 0xfffa000000000000ULL;

static const char THING_TYPE_STRING = 0;
static const char THING_TYPE_LIST = 1;
static const char THING_TYPE_OBJECT = 2;
static const char THING_TYPE_MASK = 0x7f;

/* internal value helpers */

typedef struct {
    void *payload;
    int refcount;
    char type;
} thing_t;

typedef struct {
    sentry_value_t *items;
    size_t len;
    size_t allocated;
} list_t;

typedef struct {
    char *k;
    sentry_value_t v;
} obj_pair_t;

typedef struct {
    obj_pair_t *pairs;
    size_t len;
    size_t allocated;
} obj_t;

static bool
reserve(void **buf, size_t item_size, size_t *allocated, size_t *len,
    size_t min_len)
{
    if (*allocated >= min_len) {
        return true;
    }
    size_t new_allocated = *allocated;
    if (new_allocated == 0) {
        new_allocated = 10;
    }
    while (new_allocated < min_len) {
        new_allocated *= 2;
    }

    void *new_buf = sentry_realloc(*buf, new_allocated * item_size);
    if (!new_buf) {
        return false;
    }

    *buf = new_buf;
    *allocated = new_allocated;
    return true;
}

static void
thing_free(thing_t *thing)
{
    sentry_free(thing);
}

static void
thing_freeze(thing_t *thing)
{
}

static int
thing_is_frozen(const thing_t *thing)
{
    return (thing->type >> 7) != 0;
}

static int
thing_get_type(const thing_t *thing)
{
    return thing->type & THING_TYPE_MASK;
}

static sentry_value_t
new_thing_value(void *ptr, char thing_type)
{
    sentry_value_t rv;
    thing_t *thing = sentry_malloc(sizeof(thing_t));
    if (!thing) {
        return sentry_value_new_null();
    }

    thing->payload = ptr;
    thing->refcount = 1;
    thing->type = thing_type;
    rv._bits = (((uint64_t)thing) >> 2) | TAG_THING;
    return rv;
}

static thing_t *
value_as_thing(sentry_value_t value)
{
    if (value._bits <= MAX_DOUBLE) {
        return NULL;
    } else if ((value._bits & TAG_THING) == TAG_THING) {
        return (thing_t *)((value._bits << 2) & ~TAG_THING);
    } else {
        return NULL;
    }
}

/* public api implementations */

void
sentry_value_incref(sentry_value_t value)
{
    thing_t *thing = value_as_thing(value);
    if (thing) {
        sentry__atomic_fetch_and_add(&thing->refcount, 1);
    }
}

void
sentry_value_decref(sentry_value_t value)
{
    thing_t *thing = value_as_thing(value);
    if (thing && sentry__atomic_fetch_and_add(&thing->refcount, -1) == 1) {
        thing_free(thing);
    }
}

size_t
sentry_value_refcount(sentry_value_t value)
{
    thing_t *thing = value_as_thing(value);
    return thing ? (size_t)sentry__atomic_fetch(&thing->refcount) : 1;
}

void
sentry_value_freeze(sentry_value_t value)
{
    thing_t *thing = value_as_thing(value);
    if (thing) {
        thing_freeze(thing);
    }
}

sentry_value_t
sentry_value_new_null(void)
{
    sentry_value_t rv;
    rv._bits = (uint64_t)2 | TAG_CONST;
    return rv;
}

sentry_value_t
sentry_value_new_int32(int32_t value)
{
    sentry_value_t rv;
    rv._bits = (uint64_t)(uint32_t)value | TAG_INT32;
    return rv;
}

sentry_value_t
sentry_value_new_double(double value)
{
    sentry_value_t rv;
    // if we are a nan value we want to become the max double value which
    // is a NAN.
    if (value != value) {
        rv._bits = MAX_DOUBLE;
    } else {
        rv._double = value;
    }
    return rv;
}

sentry_value_t
sentry_value_new_bool(int value)
{
    sentry_value_t rv;
    rv._bits = (uint64_t)(value ? 1 : 0) | TAG_CONST;
    return rv;
}

sentry_value_t
sentry_value_new_string(const char *value)
{
    char *s = sentry__string_dup(value);
    if (!s) {
        return sentry_value_new_null();
    }
    return sentry__value_new_string_owned(s);
}

sentry_value_t
sentry_value_new_list(void)
{
    list_t *l = SENTRY_MAKE(list_t);
    if (l) {
        memset(l, 0, sizeof(list_t));
        return new_thing_value(l, THING_TYPE_LIST);
    } else {
        return sentry_value_new_null();
    }
}

sentry_value_t
sentry_value_new_object(void)
{
    obj_t *o = SENTRY_MAKE(obj_t);
    if (o) {
        memset(o, 0, sizeof(obj_t));
        return new_thing_value(o, THING_TYPE_OBJECT);
    } else {
        return sentry_value_new_null();
    }
}

sentry_value_type_t
sentry_value_get_type(sentry_value_t value)
{
    const thing_t *thing = value_as_thing(value);
    if (thing) {
        switch (thing_get_type(thing)) {
        case THING_TYPE_STRING:
            return SENTRY_VALUE_TYPE_STRING;
        case THING_TYPE_LIST:
            return SENTRY_VALUE_TYPE_LIST;
        case THING_TYPE_OBJECT:
            return SENTRY_VALUE_TYPE_OBJECT;
        }
        assert(!"unreachable");
    } else if (value._bits <= MAX_DOUBLE) {
        return SENTRY_VALUE_TYPE_DOUBLE;
    } else if ((value._bits & TAG_CONST) == TAG_CONST) {
        uint64_t val = value._bits & ~TAG_CONST;
        switch (val) {
        case 0:
        case 1:
            return SENTRY_VALUE_TYPE_BOOL;
        case 2:
            return SENTRY_VALUE_TYPE_NULL;
        default:
            assert(!"unreachable");
        }
    } else if ((value._bits & TAG_INT32) == TAG_INT32) {
        return SENTRY_VALUE_TYPE_INT32;
    }
    return SENTRY_VALUE_TYPE_DOUBLE;
}

int
sentry_value_set_by_key(sentry_value_t value, const char *k, sentry_value_t v)
{
    thing_t *thing = value_as_thing(value);
    if (!thing || thing_get_type(thing) != THING_TYPE_OBJECT) {
        return 1;
    }
    obj_t *o = thing->payload;
    for (size_t i = 0; i < o->len; i++) {
        obj_pair_t *pair = &o->pairs[i];
        if (strcmp(pair->k, k) == 0) {
            sentry_value_decref(pair->v);
            pair->v = v;
            return 0;
        }
    }

    if (!reserve((void **)&o->pairs, sizeof(o->pairs[0]), &o->allocated,
            &o->len, o->len + 1)) {
        return 1;
    }

    obj_pair_t pair;
    pair.k = sentry__string_dup(k);
    if (!pair.k) {
        return 1;
    }
    pair.v = v;
    o->pairs[o->len++] = pair;
    return 0;
}

int
sentry_value_remove_by_key(sentry_value_t value, const char *k)
{
    thing_t *thing = value_as_thing(value);
    if (!thing || thing_get_type(thing) != THING_TYPE_OBJECT) {
        return 0;
    }
    obj_t *o = thing->payload;
    for (size_t i = 0; i < o->len; i++) {
        obj_pair_t *pair = &o->pairs[i];
        if (strcmp(pair->k, k) == 0) {
            memmove(o->pairs + i, o->pairs + i + 1,
                (o->len - i - 1) * sizeof(o->pairs[0]));
            sentry_value_decref(pair->v);
            o->len--;
            return 0;
        }
    }
    return 1;
}

int
sentry_value_append(sentry_value_t value, sentry_value_t v)
{
    thing_t *thing = value_as_thing(value);
    if (!thing || thing_get_type(thing) != THING_TYPE_LIST) {
        return 1;
    }
    list_t *l = thing->payload;

    if (!reserve((void **)&l->items, sizeof(l->items[0]), &l->allocated,
            &l->len, l->len + 1)) {
        return 1;
    }

    l->items[l->len++] = v;
    return 0;
}

int
sentry_value_set_by_index(sentry_value_t value, size_t index, sentry_value_t v)
{
    thing_t *thing = value_as_thing(value);
    if (!thing || thing_get_type(thing) != THING_TYPE_LIST) {
        return 1;
    }

    list_t *l = thing->payload;
    if (!reserve((void *)&l->items, sizeof(l->items[0]), &l->allocated, &l->len,
            index + 1)) {
        return 1;
    }

    if (index >= l->len) {
        for (size_t i = l->len; i < index + 1; i++) {
            l->items[i] = sentry_value_new_null();
        }
        l->len = index + 1;
    }

    sentry_value_decref(l->items[index]);
    l->items[index] = v;
    return 0;
}

int
sentry_value_remove_by_index(sentry_value_t value, size_t index)
{
    thing_t *thing = value_as_thing(value);
    if (!thing || thing_get_type(thing) != THING_TYPE_LIST) {
        return 1;
    }

    list_t *l = thing->payload;
    if (index >= l->len) {
        return 0;
    }

    sentry_value_decref(l->items[index]);
    memmove(l->items + index, l->items + index + 1,
        (l->len - index - 1) * sizeof(l->items[0]));
    l->len--;
    return 0;
}

sentry_value_t
sentry_value_get_by_key(sentry_value_t value, const char *k)
{
    const thing_t *thing = value_as_thing(value);
    if (thing && thing_get_type(thing) == THING_TYPE_OBJECT) {
        obj_t *o = thing->payload;
        for (size_t i = 0; i < o->len; i++) {
            obj_pair_t *pair = &o->pairs[i];
            if (strcmp(pair->k, k) == 0) {
                return pair->v;
            }
        }
    }
    return sentry_value_new_null();
}

sentry_value_t
sentry_value_get_by_key_owned(sentry_value_t value, const char *k)
{
    sentry_value_t rv = sentry_value_get_by_key(value, k);
    sentry_value_incref(rv);
    return rv;
}

sentry_value_t
sentry_value_get_by_index(sentry_value_t value, size_t index)
{
    const thing_t *thing = value_as_thing(value);
    if (thing && thing_get_type(thing) == THING_TYPE_LIST) {
        list_t *l = thing->payload;
        if (index < l->len) {
            return l->items[index];
        }
    }
    return sentry_value_new_null();
}

sentry_value_t
sentry_value_get_by_index_owned(sentry_value_t value, size_t index)
{
    sentry_value_t rv = sentry_value_get_by_index(value, index);
    sentry_value_incref(rv);
    return rv;
}

size_t
sentry_value_get_length(sentry_value_t value)
{
    const thing_t *thing = value_as_thing(value);
    if (thing) {
        switch (thing_get_type(thing)) {
        case THING_TYPE_STRING:
            return strlen(thing->payload);
        case THING_TYPE_LIST:
            return ((const list_t *)thing->payload)->len;
        case THING_TYPE_OBJECT:
            return ((const obj_t *)thing->payload)->len;
        }
    }
    return 0;
}

int32_t
sentry_value_as_int32(sentry_value_t value)
{
    if ((value._bits & TAG_INT32) == TAG_INT32) {
        return (int32_t)(value._bits & ~TAG_INT32);
    } else {
        return 0;
    }
}

double
sentry_value_as_double(sentry_value_t value)
{
    if (value._bits <= MAX_DOUBLE) {
        return value._double;
    } else if ((value._bits & TAG_INT32) == TAG_INT32) {
        return (double)sentry_value_as_int32(value);
    } else {
        return MAX_DOUBLE;
    }
}

const char *
sentry_value_as_string(sentry_value_t value)
{
    const thing_t *thing = value_as_thing(value);
    if (thing && thing_get_type(thing) == THING_TYPE_STRING) {
        return (const char *)thing->payload;
    } else {
        return "";
    }
}

int
sentry_value_is_true(sentry_value_t value)
{
    switch (sentry_value_get_type(value)) {
    case SENTRY_VALUE_TYPE_BOOL:
    case SENTRY_VALUE_TYPE_NULL:
        return (value._bits & ~TAG_CONST) == 1;
    case SENTRY_VALUE_TYPE_INT32:
        return sentry_value_as_int32(value) != 0;
    case SENTRY_VALUE_TYPE_DOUBLE:
        return sentry_value_as_double(value) != 0.0;
    default:
        return sentry_value_get_length(value) > 0;
    }
}

int
sentry_value_is_null(sentry_value_t value)
{
    if ((value._bits & TAG_CONST) == TAG_CONST) {
        uint64_t val = value._bits & ~TAG_CONST;
        return val == 2;
    }
    return false;
}

static void
value_to_json(sentry_jsonwriter_t *jw, sentry_value_t value)
{
    switch (sentry_value_get_type(value)) {
    case SENTRY_VALUE_TYPE_NULL:
        sentry__jsonwriter_write_null(jw);
        break;
    case SENTRY_VALUE_TYPE_BOOL:
        sentry__jsonwriter_write_bool(jw, sentry_value_is_true(value));
        break;
    case SENTRY_VALUE_TYPE_INT32:
        sentry__jsonwriter_write_int32(jw, sentry_value_as_int32(value));
        break;
    case SENTRY_VALUE_TYPE_DOUBLE:
        sentry__jsonwriter_write_double(jw, sentry_value_as_double(value));
        break;
    case SENTRY_VALUE_TYPE_STRING:
        sentry__jsonwriter_write_str(jw, sentry_value_as_string(value));
        break;
    case SENTRY_VALUE_TYPE_LIST: {
        const list_t *l = value_as_thing(value)->payload;
        sentry__jsonwriter_write_list_start(jw);
        for (size_t i = 0; i < l->len; i++) {
            value_to_json(jw, l->items[i]);
        }
        sentry__jsonwriter_write_list_end(jw);
        break;
    }
    case SENTRY_VALUE_TYPE_OBJECT: {
        const obj_t *o = value_as_thing(value)->payload;
        sentry__jsonwriter_write_object_start(jw);
        for (size_t i = 0; i < o->len; i++) {
            sentry__jsonwriter_write_key(jw, o->pairs[i].k);
            value_to_json(jw, o->pairs[i].v);
        }
        sentry__jsonwriter_write_object_end(jw);
        break;
    }
    }
}

char *
sentry_value_to_json(sentry_value_t value)
{
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_in_memory();
    value_to_json(jw, value);
    return sentry__jsonwriter_into_string(jw);
}

sentry_value_t
sentry__value_new_string_owned(char *s)
{
    return new_thing_value(s, THING_TYPE_STRING);
}

sentry_value_t
sentry__value_new_addr(uint64_t addr)
{
    char buf[100];
    snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)addr);
    return sentry_value_new_string(buf);
}

sentry_value_t
sentry__value_new_hexstring(const char *bytes, size_t len)
{
    char *buf = sentry_malloc(len * 2 + 1);
    if (!buf) {
        return sentry_value_new_null();
    }
    char *ptr;
    for (size_t i = 0; i < len; i++) {
        ptr += snprintf(ptr, 3, "%02hhx", bytes[i]);
    }
    return sentry__value_new_string_owned(buf);
}

sentry_value_t
sentry__value_new_uuid(const sentry_uuid_t *uuid)
{
    char *buf = sentry_malloc(37);
    if (!buf) {
        return sentry_value_new_null();
    }
    sentry_uuid_as_string(uuid, buf);
    return sentry__value_new_string_owned(buf);
}