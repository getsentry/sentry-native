#include "sentry_gpu.h"

sentry_gpu_info_t *
sentry__get_gpu_info(void)
{
    return NULL;
}

void
sentry__free_gpu_info(sentry_gpu_info_t *gpu_info)
{
    (void)gpu_info; // Unused parameter
}

char *
sentry__gpu_vendor_id_to_name(unsigned int vendor_id)
{
    (void)vendor_id; // Unused parameter
    return NULL;
}

sentry_value_t
sentry__get_gpu_context(void)
{
    return sentry_value_new_null();
}