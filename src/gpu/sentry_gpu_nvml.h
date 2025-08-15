#ifndef SENTRY_GPU_NVML_H_INCLUDED
#define SENTRY_GPU_NVML_H_INCLUDED

#include "sentry_gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NVML_SUCCESS 0
#define NVML_DEVICE_NAME_BUFFER_SIZE 64
#define NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE 80

typedef enum nvmlReturn_enum {
    NVML_SUCCESS_VALUE = 0,
} nvmlReturn_t;

typedef void *nvmlDevice_t;

typedef struct {
    void *handle;
    nvmlReturn_t (*nvmlInit)(void);
    nvmlReturn_t (*nvmlShutdown)(void);
    nvmlReturn_t (*nvmlDeviceGetCount)(unsigned int *);
    nvmlReturn_t (*nvmlDeviceGetHandleByIndex)(unsigned int, nvmlDevice_t *);
    nvmlReturn_t (*nvmlDeviceGetName)(nvmlDevice_t, char *, unsigned int);
    nvmlReturn_t (*nvmlDeviceGetMemoryInfo)(nvmlDevice_t, void *);
    nvmlReturn_t (*nvmlSystemGetDriverVersion)(char *, unsigned int);
} nvml_api_t;

typedef struct {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
} nvml_memory_t;

/**
 * Retrieves information for all NVIDIA GPUs using NVML.
 * Returns a sentry_gpu_list_t structure that must be freed with
 * sentry__free_gpu_list, or NULL if no NVIDIA GPUs or NVML is unavailable.
 */
sentry_gpu_list_t *sentry__get_gpu_info_nvidia_nvml(void);

#ifdef __cplusplus
}
#endif

#endif
