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
    default:
        return sentry__string_clone("Unknown");
    }
}

sentry_value_t
sentry__get_gpu_context(void)
{
    sentry_gpu_info_t *gpu_info = sentry__get_gpu_info();
    if (!gpu_info) {
        return sentry_value_new_null();
    }

    sentry_value_t gpu_context = sentry_value_new_object();
    if (sentry_value_is_null(gpu_context)) {
        sentry__free_gpu_info(gpu_info);
        return gpu_context;
    }

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
        sentry_value_set_by_key(gpu_context, "vendor_id",
            sentry_value_new_int32((int32_t)gpu_info->vendor_id));
    }

    // Add device ID
    if (gpu_info->device_id != 0) {
        sentry_value_set_by_key(gpu_context, "device_id",
            sentry_value_new_int32((int32_t)gpu_info->device_id));
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

    sentry__free_gpu_info(gpu_info);
    sentry_value_freeze(gpu_context);
    return gpu_context;
}
