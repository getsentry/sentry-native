#include "sentry_gpu.h"
#include "sentry_string.h"

char *
sentry__gpu_vendor_id_to_name(unsigned int vendor_id)
{
    switch (vendor_id) {
    case 0x10DE:
        return sentry__string_clone("NVIDIA Corporation");
    case 0x1002:
        return sentry__string_clone("Advanced Micro Devices, Inc. [AMD/ATI]");
    case 0x8086:
        return sentry__string_clone("Intel Corporation");
    case 0x106B:
        return sentry__string_clone("Apple Inc.");
    case 0x1414:
        return sentry__string_clone("Microsoft Corporation");
    case 0x5143:
        return sentry__string_clone("Qualcomm");
    case 0x1AE0:
        return sentry__string_clone("Google");
    case 0x1010:
        return sentry__string_clone("VideoLogic");
    case 0x1023:
        return sentry__string_clone("Trident Microsystems");
    case 0x102B:
        return sentry__string_clone("Matrox Graphics");
    case 0x121A:
        return sentry__string_clone("3dfx Interactive");
    case 0x18CA:
        return sentry__string_clone("XGI Technology");
    case 0x1039:
        return sentry__string_clone("Silicon Integrated Systems [SiS]");
    case 0x126F:
        return sentry__string_clone("Silicon Motion");
    default: {
        char unknown_vendor[64];
        snprintf(unknown_vendor, sizeof(unknown_vendor), "Unknown (0x%04X)",
            vendor_id);
        return sentry__string_clone(unknown_vendor);
    }
    }
}

static sentry_value_t
create_gpu_context_from_info(sentry_gpu_info_t *gpu_info)
{
    sentry_value_t gpu_context = sentry_value_new_object();
    if (sentry_value_is_null(gpu_context)) {
        return gpu_context;
    }

    // Add type field for frontend recognition
    sentry_value_set_by_key(
        gpu_context, "type", sentry_value_new_string("gpu"));

    // Add GPU name
    if (gpu_info->name) {
        sentry_value_set_by_key(
            gpu_context, "name", sentry_value_new_string(gpu_info->name));
    }

    // Add vendor information
    if (gpu_info->vendor_name) {
        sentry_value_set_by_key(gpu_context, "vendor_name",
            sentry_value_new_string(gpu_info->vendor_name));
    }

    if (gpu_info->vendor_id != 0) {
        char vendor_id_str[32];
        snprintf(
            vendor_id_str, sizeof(vendor_id_str), "%u", gpu_info->vendor_id);
        sentry_value_set_by_key(
            gpu_context, "vendor_id", sentry_value_new_string(vendor_id_str));
    }

    // Add device ID
    if (gpu_info->device_id != 0) {
        char device_id_str[32];
        snprintf(
            device_id_str, sizeof(device_id_str), "%u", gpu_info->device_id);
        sentry_value_set_by_key(
            gpu_context, "device_id", sentry_value_new_string(device_id_str));
    }

    // Add memory size
    if (gpu_info->memory_size > 0) {
        sentry_value_set_by_key(gpu_context, "memory_size",
            sentry_value_new_int64((int64_t)gpu_info->memory_size));
    }

    // Add driver version
    if (gpu_info->driver_version) {
        sentry_value_set_by_key(gpu_context, "driver_version",
            sentry_value_new_string(gpu_info->driver_version));
    }

    sentry_value_freeze(gpu_context);
    return gpu_context;
}

void
sentry__free_gpu_info(sentry_gpu_info_t *gpu_info)
{
    if (!gpu_info) {
        return;
    }

    sentry_free(gpu_info->name);
    sentry_free(gpu_info->vendor_name);
    sentry_free(gpu_info->driver_version);
    sentry_free(gpu_info);
}

void
sentry__free_gpu_list(sentry_gpu_list_t *gpu_list)
{
    if (!gpu_list) {
        return;
    }

    for (unsigned int i = 0; i < gpu_list->count; i++) {
        sentry__free_gpu_info(gpu_list->gpus[i]);
    }

    sentry_free(gpu_list->gpus);
    sentry_free(gpu_list);
}

void
sentry__add_gpu_contexts(sentry_value_t contexts)
{
    sentry_gpu_list_t *gpu_list = sentry__get_gpu_info();
    if (!gpu_list) {
        return;
    }

    for (unsigned int i = 0; i < gpu_list->count; i++) {
        sentry_value_t gpu_context
            = create_gpu_context_from_info(gpu_list->gpus[i]);
        if (!sentry_value_is_null(gpu_context)) {
            char context_key[16];
            if (i == 0) {
                snprintf(context_key, sizeof(context_key), "gpu");
            } else {
                snprintf(context_key, sizeof(context_key), "gpu%u", i + 1);
            }
            sentry_value_set_by_key(contexts, context_key, gpu_context);
        }
    }

    sentry__free_gpu_list(gpu_list);
}
