#include "sentry.h"
#include <stdlib.h>

static sentry_value_t
before_send_metric(sentry_value_t metric, void *user_data)
{
    (void)user_data;

    sentry_value_t attributes = sentry_value_get_by_key(metric, "attributes");
    sentry_value_remove_by_key(attributes, "task.id");
    return metric;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);
    sentry_options_set_before_send_metric(options, before_send_metric, NULL);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_value_t attributes = sentry_value_new_object();
    sentry_value_set_by_key(attributes, "task.type",
        sentry_value_new_attribute(sentry_value_new_string("import"), NULL));
    sentry_value_set_by_key(attributes, "task.id",
        sentry_value_new_attribute(sentry_value_new_string("task-123"), NULL));
    sentry_metrics_count("tasks.completed", 1, attributes);
    sentry_metrics_gauge(
        "memory.used", 1048576, SENTRY_UNIT_BYTE, sentry_value_new_null());
    sentry_metrics_distribution("task.duration", 12.5, SENTRY_UNIT_MILLISECOND,
        sentry_value_new_null());

    sentry_close();
}
