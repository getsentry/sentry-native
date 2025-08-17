#ifndef SENTRY_GPU_VULKAN_H_INCLUDED
#define SENTRY_GPU_VULKAN_H_INCLUDED

#include "sentry_gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves GPU information using Vulkan API.
 * Returns a sentry_gpu_list_t structure that must be freed with
 * sentry__free_gpu_list, or NULL if no GPU information could be obtained.
 */
sentry_gpu_list_t *sentry__get_gpu_info(void);
#endif

#ifdef __cplusplus
}
#endif
