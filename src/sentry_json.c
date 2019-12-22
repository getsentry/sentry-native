#include <stdio.h>
#include <string.h>

#include "sentry_alloc.h"
#include "sentry_json.h"
#include "sentry_string.h"

#define DST_MODE_SB 1

struct sentry__jsonwriter_s {
    union {
        sentry__stringbuilder_t *sb;
    } dst;
    uint64_t want_comma;
    uint32_t depth;
    bool last_was_key;
    char dst_mode;
};

sentry__jsonwriter_t *
sentry__jsonwriter_new_in_memory(void)
{
    sentry__jsonwriter_t *rv = SENTRY_MAKE(sentry__jsonwriter_t);
    if (!rv) {
        return NULL;
    }
    rv->dst.sb = SENTRY_MAKE(sentry__stringbuilder_t);
    if (!rv->dst.sb) {
        sentry_free(rv);
        return NULL;
    }
    sentry__stringbuilder_init(rv->dst.sb);
    rv->dst_mode = DST_MODE_SB;
    rv->want_comma = 0;
    rv->depth = 0;
    rv->last_was_key = 0;
    return rv;
}

void
sentry__jsonwriter_free(sentry__jsonwriter_t *jw)
{
    if (!jw) {
        return;
    }
    switch (jw->dst_mode) {
    case DST_MODE_SB:
        sentry__stringbuilder_cleanup(jw->dst.sb);
    }
    sentry_free(jw);
}

char *
sentry__jsonwriter_into_string(sentry__jsonwriter_t *jw)
{
    switch (jw->dst_mode) {
    case DST_MODE_SB:
        return sentry__stringbuilder_into_string(jw->dst.sb);
    }
    return NULL;
}

static bool
at_max_depth(const sentry__jsonwriter_t *jw)
{
    return jw->depth >= 64;
}

static void
set_comma(sentry__jsonwriter_t *jw, bool val)
{
    if (at_max_depth(jw)) {
        return;
    }
    if (val) {
        jw->want_comma |= 1ULL << jw->depth;
    } else {
        jw->want_comma &= ~(1ULL << jw->depth);
    }
}

static void
write_char(sentry__jsonwriter_t *jw, char c)
{
    switch (jw->dst_mode) {
    case DST_MODE_SB:
        sentry__stringbuilder_append_char(jw->dst.sb, c);
    }
}

static void
write_str(sentry__jsonwriter_t *jw, const char *str)
{
    switch (jw->dst_mode) {
    case DST_MODE_SB:
        sentry__stringbuilder_append(jw->dst.sb, str);
    }
}

static void
write_json_str(sentry__jsonwriter_t *jw, const char *str)
{
    const char *ptr = str;
    write_char(jw, '"');
    for (; *ptr; ptr++) {
        switch (*ptr) {
        case '\\':
            write_str(jw, "\\\\");
            break;
        case '"':
            write_str(jw, "\\\"");
            break;
        case '\b':
            write_str(jw, "\\b");
            break;
        case '\f':
            write_str(jw, "\\f");
            break;
        case '\n':
            write_str(jw, "\\n");
            break;
        case '\r':
            write_str(jw, "\\r");
            break;
        case '\t':
            write_str(jw, "\\t");
            break;
        default:
            if (*ptr < 32) {
                char buf[10];
                snprintf(buf, sizeof(buf), "u%04x", *ptr);
                write_str(jw, buf);
            } else {
                write_char(jw, *ptr);
            }
        }
    }
    write_char(jw, '"');
}

static bool
can_write_item(sentry__jsonwriter_t *jw)
{
    if (at_max_depth(jw)) {
        return false;
    }
    if (jw->last_was_key) {
        jw->last_was_key = false;
        return true;
    }
    if ((jw->want_comma >> jw->depth) & 1) {
        write_char(jw, ',');
    } else {
        set_comma(jw, true);
    }
    return true;
}

void
sentry__jsonwriter_write_null(sentry__jsonwriter_t *jw)
{
    if (can_write_item(jw)) {
        write_str(jw, "null");
    }
}

void
sentry__jsonwriter_write_bool(sentry__jsonwriter_t *jw, bool val)
{
    if (can_write_item(jw)) {
        write_str(jw, val ? "true" : "false");
    }
}

void
sentry__jsonwriter_write_int32(sentry__jsonwriter_t *jw, int32_t val)
{
    if (can_write_item(jw)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%" PRId32, val);
        write_str(jw, buf);
    }
}

void
sentry__jsonwriter_write_double(sentry__jsonwriter_t *jw, double val)
{
    if (can_write_item(jw)) {
        char buf[50];
        snprintf(buf, sizeof(buf), "%g", val);
        write_str(jw, buf);
    }
}

void
sentry__jsonwriter_write_str(sentry__jsonwriter_t *jw, const char *val)
{
    if (can_write_item(jw)) {
        write_json_str(jw, val);
    }
}

void
sentry__jsonwriter_write_key(sentry__jsonwriter_t *jw, const char *val)
{
    if (can_write_item(jw)) {
        write_json_str(jw, val);
        write_char(jw, ':');
        jw->last_was_key = true;
    }
}

void
sentry__jsonwriter_write_list_start(sentry__jsonwriter_t *jw)
{
    if (!can_write_item(jw)) {
        return;
    }
    write_char(jw, '[');
    jw->depth += 1;
    set_comma(jw, false);
}

void
sentry__jsonwriter_write_list_end(sentry__jsonwriter_t *jw)
{
    write_char(jw, ']');
    jw->depth -= 1;
}

void
sentry__jsonwriter_write_object_start(sentry__jsonwriter_t *jw)
{
    if (!can_write_item(jw)) {
        return;
    }
    write_char(jw, '{');
    jw->depth += 1;
    set_comma(jw, false);
}

void
sentry__jsonwriter_write_object_end(sentry__jsonwriter_t *jw)
{
    write_char(jw, '}');
    jw->depth -= 1;
}