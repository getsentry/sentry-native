#include "sentry_scope.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(mpack_removed_tags)
{
    sentry_value_t obj = sentry_value_new_object();

    sentry_set_tag("foo", "foo");
    sentry_set_tag("bar", "bar");
    sentry_set_tag("baz", "baz");
    sentry_set_tag("qux", "qux");
    sentry_remove_tag("bar");

    SENTRY_WITH_SCOPE (scope) {
        sentry__scope_apply_to_event(scope, obj, SENTRY_SCOPE_NONE);
    }

    size_t size;
    char *buf = sentry_value_to_msgpack(obj, &size);

    sentry_value_decref(obj);
    sentry_free(buf);
    sentry__scope_cleanup();
}
