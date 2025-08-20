#include "sentry_alloc.h"
#include "sentry_gpu.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <string.h>
#include <vulkan/vulkan.h>

static VkInstance
create_vulkan_instance(void)
{
    VkApplicationInfo app_info = { 0 };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Sentry GPU Info";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Sentry";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        SENTRY_DEBUGF("Failed to create Vulkan instance: %d", result);
        return VK_NULL_HANDLE;
    }

    return instance;
}

static sentry_gpu_info_t *
create_gpu_info_from_device(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

    sentry_gpu_info_t *gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
    if (!gpu_info) {
        return NULL;
    }

    memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

    gpu_info->name = sentry__string_clone(properties.deviceName);
    gpu_info->vendor_id = properties.vendorID;
    gpu_info->device_id = properties.deviceID;
    gpu_info->vendor_name = sentry__gpu_vendor_id_to_name(properties.vendorID);

    char driver_version_str[64];
    uint32_t driver_version = properties.driverVersion;
    snprintf(driver_version_str, sizeof(driver_version_str), "%u.%u.%u",
        VK_VERSION_MAJOR(driver_version), VK_VERSION_MINOR(driver_version),
        VK_VERSION_PATCH(driver_version));

    gpu_info->driver_version = sentry__string_clone(driver_version_str);

    size_t total_memory = 0;
    for (uint32_t i = 0; i < memory_properties.memoryHeapCount; i++) {
        if (memory_properties.memoryHeaps[i].flags
            & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            total_memory += memory_properties.memoryHeaps[i].size;
        }
    }
    gpu_info->memory_size = total_memory;

    return gpu_info;
}

sentry_gpu_list_t *
sentry__get_gpu_info(void)
{
    VkInstance instance = create_vulkan_instance();
    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (result != VK_SUCCESS || device_count == 0) {
        SENTRY_DEBUGF("Failed to enumerate Vulkan devices: %d", result);
        vkDestroyInstance(instance, NULL);
        return NULL;
    }

    VkPhysicalDevice *devices
        = sentry_malloc(sizeof(VkPhysicalDevice) * device_count);
    if (!devices) {
        vkDestroyInstance(instance, NULL);
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(instance, &device_count, devices);
    if (result != VK_SUCCESS) {
        SENTRY_DEBUGF("Failed to get Vulkan physical devices: %d", result);
        sentry_free(devices);
        vkDestroyInstance(instance, NULL);
        return NULL;
    }

    sentry_gpu_list_t *gpu_list = sentry_malloc(sizeof(sentry_gpu_list_t));
    if (!gpu_list) {
        sentry_free(devices);
        vkDestroyInstance(instance, NULL);
        return NULL;
    }

    gpu_list->gpus = sentry_malloc(sizeof(sentry_gpu_info_t *) * device_count);
    if (!gpu_list->gpus) {
        sentry_free(gpu_list);
        sentry_free(devices);
        vkDestroyInstance(instance, NULL);
        return NULL;
    }

    gpu_list->count = 0;

    for (uint32_t i = 0; i < device_count; i++) {
        sentry_gpu_info_t *gpu_info = create_gpu_info_from_device(devices[i]);
        if (gpu_info) {
            gpu_list->gpus[gpu_list->count] = gpu_info;
            gpu_list->count++;
        }
    }

    sentry_free(devices);
    vkDestroyInstance(instance, NULL);

    if (gpu_list->count == 0) {
        sentry_free(gpu_list->gpus);
        sentry_free(gpu_list);
        return NULL;
    }

    return gpu_list;
}
