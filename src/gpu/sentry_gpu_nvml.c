#include "sentry_gpu_nvml.h"

#include "sentry_alloc.h"
#include "sentry_string.h"

#include <string.h>

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

static nvml_api_t *
load_nvml(void)
{
    nvml_api_t *nvml = sentry_malloc(sizeof(nvml_api_t));
    if (!nvml) {
        return NULL;
    }

    memset(nvml, 0, sizeof(nvml_api_t));

#ifdef SENTRY_PLATFORM_WINDOWS
    nvml->handle = LoadLibraryA("nvml.dll");
    if (!nvml->handle) {
        sentry_free(nvml);
        return NULL;
    }

    nvml->nvmlInit
        = (nvmlReturn_t (*)(void))GetProcAddress(nvml->handle, "nvmlInit_v2");
    if (!nvml->nvmlInit) {
        nvml->nvmlInit
            = (nvmlReturn_t (*)(void))GetProcAddress(nvml->handle, "nvmlInit");
    }

    nvml->nvmlShutdown
        = (nvmlReturn_t (*)(void))GetProcAddress(nvml->handle, "nvmlShutdown");
    nvml->nvmlDeviceGetCount = (nvmlReturn_t (*)(unsigned int *))GetProcAddress(
        nvml->handle, "nvmlDeviceGetCount_v2");
    if (!nvml->nvmlDeviceGetCount) {
        nvml->nvmlDeviceGetCount = (nvmlReturn_t (*)(
            unsigned int *))GetProcAddress(nvml->handle, "nvmlDeviceGetCount");
    }

    nvml->nvmlDeviceGetHandleByIndex
        = (nvmlReturn_t (*)(unsigned int, nvmlDevice_t *))GetProcAddress(
            nvml->handle, "nvmlDeviceGetHandleByIndex_v2");
    if (!nvml->nvmlDeviceGetHandleByIndex) {
        nvml->nvmlDeviceGetHandleByIndex
            = (nvmlReturn_t (*)(unsigned int, nvmlDevice_t *))GetProcAddress(
                nvml->handle, "nvmlDeviceGetHandleByIndex");
    }

    nvml->nvmlDeviceGetName = (nvmlReturn_t (*)(nvmlDevice_t, char *,
        unsigned int))GetProcAddress(nvml->handle, "nvmlDeviceGetName");
    nvml->nvmlDeviceGetMemoryInfo = (nvmlReturn_t (*)(nvmlDevice_t,
        void *))GetProcAddress(nvml->handle, "nvmlDeviceGetMemoryInfo");
    nvml->nvmlSystemGetDriverVersion
        = (nvmlReturn_t (*)(char *, unsigned int))GetProcAddress(
            nvml->handle, "nvmlSystemGetDriverVersion");
#else
    nvml->handle = dlopen("libnvidia-ml.so.1", RTLD_LAZY);
    if (!nvml->handle) {
        nvml->handle = dlopen("libnvidia-ml.so", RTLD_LAZY);
    }

    if (!nvml->handle) {
        sentry_free(nvml);
        return NULL;
    }

    nvml->nvmlInit = dlsym(nvml->handle, "nvmlInit");
    nvml->nvmlShutdown = dlsym(nvml->handle, "nvmlShutdown");
    nvml->nvmlDeviceGetCount = dlsym(nvml->handle, "nvmlDeviceGetCount");
    nvml->nvmlDeviceGetHandleByIndex
        = dlsym(nvml->handle, "nvmlDeviceGetHandleByIndex");
    nvml->nvmlDeviceGetName = dlsym(nvml->handle, "nvmlDeviceGetName");
    nvml->nvmlDeviceGetMemoryInfo
        = dlsym(nvml->handle, "nvmlDeviceGetMemoryInfo");
    nvml->nvmlSystemGetDriverVersion
        = dlsym(nvml->handle, "nvmlSystemGetDriverVersion");
#endif

    if (!nvml->nvmlInit || !nvml->nvmlShutdown || !nvml->nvmlDeviceGetCount
        || !nvml->nvmlDeviceGetHandleByIndex || !nvml->nvmlDeviceGetName) {
#ifdef SENTRY_PLATFORM_WINDOWS
        FreeLibrary(nvml->handle);
#else
        dlclose(nvml->handle);
#endif
        sentry_free(nvml);
        return NULL;
    }

    return nvml;
}

static void
unload_nvml(nvml_api_t *nvml)
{
    if (!nvml) {
        return;
    }

    if (nvml->nvmlShutdown) {
        nvml->nvmlShutdown();
    }

    if (nvml->handle) {
#ifdef SENTRY_PLATFORM_WINDOWS
        FreeLibrary(nvml->handle);
#else
        dlclose(nvml->handle);
#endif
    }

    sentry_free(nvml);
}

sentry_gpu_list_t *
sentry__get_gpu_info_nvidia_nvml(void)
{
    nvml_api_t *nvml = load_nvml();
    if (!nvml) {
        return NULL;
    }

    if (nvml->nvmlInit() != NVML_SUCCESS) {
        unload_nvml(nvml);
        return NULL;
    }

    unsigned int device_count = 0;
    if (nvml->nvmlDeviceGetCount(&device_count) != NVML_SUCCESS
        || device_count == 0) {
        unload_nvml(nvml);
        return NULL;
    }

    sentry_gpu_list_t *gpu_list = sentry_malloc(sizeof(sentry_gpu_list_t));
    if (!gpu_list) {
        unload_nvml(nvml);
        return NULL;
    }

    gpu_list->gpus = sentry_malloc(sizeof(sentry_gpu_info_t *) * device_count);
    if (!gpu_list->gpus) {
        sentry_free(gpu_list);
        unload_nvml(nvml);
        return NULL;
    }

    gpu_list->count = 0;

    for (unsigned int i = 0; i < device_count; i++) {
        nvmlDevice_t device;
        if (nvml->nvmlDeviceGetHandleByIndex(i, &device) != NVML_SUCCESS) {
            continue;
        }

        sentry_gpu_info_t *gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
        if (!gpu_info) {
            continue;
        }

        memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

        char name[NVML_DEVICE_NAME_BUFFER_SIZE];
        if (nvml->nvmlDeviceGetName(device, name, sizeof(name))
            == NVML_SUCCESS) {
            gpu_info->name = sentry__string_clone(name);
        }

        char driver_version[NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE];
        if (i == 0 && nvml->nvmlSystemGetDriverVersion
            && nvml->nvmlSystemGetDriverVersion(
                   driver_version, sizeof(driver_version))
                == NVML_SUCCESS) {
            gpu_info->driver_version = sentry__string_clone(driver_version);
        }

        if (nvml->nvmlDeviceGetMemoryInfo) {
            nvml_memory_t memory_info;
            if (nvml->nvmlDeviceGetMemoryInfo(device, &memory_info)
                == NVML_SUCCESS) {
                gpu_info->memory_size = memory_info.total;
            }
        }

        gpu_info->vendor_id = 0x10de; // NVIDIA vendor ID
        gpu_info->vendor_name = sentry__string_clone("NVIDIA Corporation");

        gpu_list->gpus[gpu_list->count] = gpu_info;
        gpu_list->count++;
    }

    unload_nvml(nvml);

    if (gpu_list->count == 0) {
        sentry_free(gpu_list->gpus);
        sentry_free(gpu_list);
        return NULL;
    }

    return gpu_list;
}
