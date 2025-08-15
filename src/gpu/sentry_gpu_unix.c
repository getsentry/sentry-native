#include "sentry_gpu.h"

#include "sentry_alloc.h"
#include "sentry_gpu_nvml.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef SENTRY_PLATFORM_LINUX
#    include <dirent.h>
#    include <dlfcn.h>
#    include <sys/stat.h>
#endif

#ifdef SENTRY_PLATFORM_MACOS
#    include <sys/sysctl.h>
#endif

#ifdef SENTRY_PLATFORM_MACOS
static char *
get_apple_chip_name(void)
{
    size_t size = 0;
    sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
    if (size == 0) {
        return NULL;
    }

    char *brand_string = sentry_malloc(size);
    if (!brand_string) {
        return NULL;
    }

    if (sysctlbyname("machdep.cpu.brand_string", brand_string, &size, NULL, 0)
            != 0
        || strstr(brand_string, "Apple M") == NULL) {
        sentry_free(brand_string);
        return NULL;
    } else {
        return brand_string;
    }

    sentry_free(brand_string);
    return NULL;
}

static size_t
get_system_memory_size(void)
{
    size_t memory_size = 0;
    size_t size = sizeof(memory_size);

    if (sysctlbyname("hw.memsize", &memory_size, &size, NULL, 0) == 0) {
        return memory_size;
    }

    return 0;
}

static sentry_gpu_info_t *
get_gpu_info_macos_agx(void)
{
    sentry_gpu_info_t *gpu_info = NULL;
    char *chip_name = get_apple_chip_name();
    if (!chip_name) {
        return NULL;
    }

    gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
    if (!gpu_info) {
        sentry_free(chip_name);
        return NULL;
    }

    memset(gpu_info, 0, sizeof(sentry_gpu_info_t));
    gpu_info->driver_version = sentry__string_clone("Apple AGX Driver");
    gpu_info->memory_size
        = get_system_memory_size(); // Unified memory architecture
    gpu_info->name = chip_name;
    gpu_info->vendor_name = sentry__string_clone("Apple Inc.");
    gpu_info->vendor_id = 0x106B; // Apple vendor ID

    return gpu_info;
}

static sentry_gpu_info_t *
get_gpu_info_macos(void)
{
    sentry_gpu_info_t *gpu_info = NULL;

    // Try Apple Silicon GPU first
    gpu_info = get_gpu_info_macos_agx();
    return gpu_info;
}
#endif

sentry_gpu_list_t *
sentry__get_gpu_info(void)
{
    sentry_gpu_list_t *gpu_list = NULL;
#ifdef SENTRY_PLATFORM_LINUX
    // Try NVML first for NVIDIA GPUs
    gpu_list = sentry__get_gpu_info_nvidia_nvml();
    if (!gpu_list) {
        return NULL;
    }
#endif

#ifdef SENTRY_PLATFORM_MACOS
    gpu_list = sentry_malloc(sizeof(sentry_gpu_list_t));
    if (!gpu_list) {
        return NULL;
    }

    gpu_list->gpus = NULL;
    gpu_list->count = 0;

    // For macOS, we typically have one integrated GPU
    sentry_gpu_info_t *macos_gpu = get_gpu_info_macos();
    if (macos_gpu) {
        gpu_list->gpus = sentry_malloc(sizeof(sentry_gpu_info_t *));
        if (gpu_list->gpus) {
            gpu_list->gpus[0] = macos_gpu;
            gpu_list->count = 1;
        } else {
            sentry__free_gpu_info(macos_gpu);
        }
    } else {
        sentry_free(gpu_list);
        return NULL;
    }
#endif

    return gpu_list;
}
