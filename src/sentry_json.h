#ifndef SENTRY_JSON_H_INCLUDED
#define SENTRY_JSON_H_INCLUDED

#include <sentry.h>

struct sentry__jsonwriter_s;
typedef struct sentry__jsonwriter_s sentry__jsonwriter_t;

sentry__jsonwriter_t *sentry__jsonwriter_new_in_memory(void);
void sentry__jsonwriter_free(sentry__jsonwriter_t *jw);
char *sentry__jsonwriter_into_string(sentry__jsonwriter_t *jw);

void sentry__jsonwriter_write_null(sentry__jsonwriter_t *jw);
void sentry__jsonwriter_write_bool(sentry__jsonwriter_t *jw, bool val);
void sentry__jsonwriter_write_int32(sentry__jsonwriter_t *jw, int32_t val);
void sentry__jsonwriter_write_double(sentry__jsonwriter_t *jw, double val);
void sentry__jsonwriter_write_str(sentry__jsonwriter_t *jw, const char *val);
void sentry__jsonwriter_write_key(sentry__jsonwriter_t *jw, const char *val);
void sentry__jsonwriter_write_list_start(sentry__jsonwriter_t *jw);
void sentry__jsonwriter_write_list_end(sentry__jsonwriter_t *jw);
void sentry__jsonwriter_write_object_start(sentry__jsonwriter_t *jw);
void sentry__jsonwriter_write_object_end(sentry__jsonwriter_t *jw);

#endif