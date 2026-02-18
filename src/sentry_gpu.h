#ifndef SENTRY_GPU_H_INCLUDED
#define SENTRY_GPU_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sentry_gpu_info_s {
    char *name;
    char *vendor_name;
    char *driver_version;
    unsigned int vendor_id;
    unsigned int device_id;
    uint64_t memory_size;
} sentry_gpu_info_t;

typedef struct sentry_gpu_list_s {
    sentry_gpu_info_t **gpus;
    unsigned int count;
} sentry_gpu_list_t;

/**
 * Retrieves GPU information for the current system.
 * Returns a sentry_gpu_info_t structure that must be freed with
 * sentry__free_gpu_info, or NULL if no GPU information could be obtained.
 */
sentry_gpu_list_t *sentry__get_gpu_info(void);

/**
 * Frees the GPU information structure returned by sentry__get_gpu_info.
 */
void sentry__free_gpu_info(sentry_gpu_info_t *gpu_info);

/**
 * Frees the GPU list structure returned by sentry__get_all_gpu_info.
 */
void sentry__free_gpu_list(sentry_gpu_list_t *gpu_list);

/**
 * Maps a GPU vendor ID to a vendor name string.
 * Returns a newly allocated string that must be freed, or NULL if unknown.
 */
char *sentry__gpu_vendor_id_to_name(unsigned int vendor_id);

/**
 * Adds GPU context information to the provided contexts object.
 * Creates individual contexts named "gpu", "gpu2", "gpu3", etc. for each GPU.
 */
void sentry__add_gpu_contexts(sentry_value_t contexts);

#ifdef __cplusplus
}
#endif

#endif
