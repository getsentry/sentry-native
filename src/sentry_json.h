#ifndef SENTRY_JSON_H_INCLUDED
#define SENTRY_JSON_H_INCLUDED

#include "sentry_boot.h"

struct sentry_jsonwriter_s;
typedef struct sentry_jsonwriter_s sentry_jsonwriter_t;

sentry_jsonwriter_t *sentry__jsonwriter_new_in_memory(void);
void sentry__jsonwriter_free(sentry_jsonwriter_t *jw);
char *sentry__jsonwriter_into_string(sentry_jsonwriter_t *jw);

void sentry__jsonwriter_write_null(sentry_jsonwriter_t *jw);
void sentry__jsonwriter_write_bool(sentry_jsonwriter_t *jw, bool val);
void sentry__jsonwriter_write_int32(sentry_jsonwriter_t *jw, int32_t val);
void sentry__jsonwriter_write_double(sentry_jsonwriter_t *jw, double val);
void sentry__jsonwriter_write_str(sentry_jsonwriter_t *jw, const char *val);
void sentry__jsonwriter_write_key(sentry_jsonwriter_t *jw, const char *val);
void sentry__jsonwriter_write_list_start(sentry_jsonwriter_t *jw);
void sentry__jsonwriter_write_list_end(sentry_jsonwriter_t *jw);
void sentry__jsonwriter_write_object_start(sentry_jsonwriter_t *jw);
void sentry__jsonwriter_write_object_end(sentry_jsonwriter_t *jw);

#endif
