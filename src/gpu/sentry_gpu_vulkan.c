#include "sentry_alloc.h"
#include "sentry_gpu.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <string.h>
#include <vulkan/vulkan.h>

#ifdef _WIN32
#    include <windows.h>
#    define SENTRY_LIBRARY_HANDLE HMODULE
#    define SENTRY_LOAD_LIBRARY(name) LoadLibraryA(name)
#    define SENTRY_GET_PROC_ADDRESS(handle, name) GetProcAddress(handle, name)
#    define SENTRY_FREE_LIBRARY(handle) FreeLibrary(handle)
#elif defined(__APPLE__)
#    include <dlfcn.h>
#    define SENTRY_LIBRARY_HANDLE void *
#    define SENTRY_LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
#    define SENTRY_GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
#    define SENTRY_FREE_LIBRARY(handle) dlclose(handle)
#else
#    include <dlfcn.h>
#    define SENTRY_LIBRARY_HANDLE void *
#    define SENTRY_LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
#    define SENTRY_GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
#    define SENTRY_FREE_LIBRARY(handle) dlclose(handle)
#endif

// Dynamic function pointers
static PFN_vkCreateInstance pfn_vkCreateInstance = NULL;
static PFN_vkDestroyInstance pfn_vkDestroyInstance = NULL;
static PFN_vkEnumeratePhysicalDevices pfn_vkEnumeratePhysicalDevices = NULL;
static PFN_vkGetPhysicalDeviceProperties pfn_vkGetPhysicalDeviceProperties
    = NULL;
static PFN_vkGetPhysicalDeviceMemoryProperties
    pfn_vkGetPhysicalDeviceMemoryProperties
    = NULL;

static SENTRY_LIBRARY_HANDLE vulkan_library = NULL;

static bool
load_vulkan_library(void)
{
    if (vulkan_library != NULL) {
        return true;
    }

#ifdef _WIN32
    vulkan_library = SENTRY_LOAD_LIBRARY("vulkan-1.dll");
#elif defined(__APPLE__)
    vulkan_library = SENTRY_LOAD_LIBRARY("libvulkan.1.dylib");
    if (!vulkan_library) {
        vulkan_library = SENTRY_LOAD_LIBRARY("libvulkan.dylib");
    }
    if (!vulkan_library) {
        vulkan_library
            = SENTRY_LOAD_LIBRARY("/usr/local/lib/libvulkan.1.dylib");
    }
#else
    vulkan_library = SENTRY_LOAD_LIBRARY("libvulkan.so.1");
    if (!vulkan_library) {
        vulkan_library = SENTRY_LOAD_LIBRARY("libvulkan.so");
    }
#endif

    if (!vulkan_library) {
        SENTRY_DEBUG("Failed to load Vulkan library");
        return false;
    }

    // Load function pointers
    pfn_vkCreateInstance = (PFN_vkCreateInstance)SENTRY_GET_PROC_ADDRESS(
        vulkan_library, "vkCreateInstance");
    pfn_vkDestroyInstance = (PFN_vkDestroyInstance)SENTRY_GET_PROC_ADDRESS(
        vulkan_library, "vkDestroyInstance");
    pfn_vkEnumeratePhysicalDevices
        = (PFN_vkEnumeratePhysicalDevices)SENTRY_GET_PROC_ADDRESS(
            vulkan_library, "vkEnumeratePhysicalDevices");
    pfn_vkGetPhysicalDeviceProperties
        = (PFN_vkGetPhysicalDeviceProperties)SENTRY_GET_PROC_ADDRESS(
            vulkan_library, "vkGetPhysicalDeviceProperties");
    pfn_vkGetPhysicalDeviceMemoryProperties
        = (PFN_vkGetPhysicalDeviceMemoryProperties)SENTRY_GET_PROC_ADDRESS(
            vulkan_library, "vkGetPhysicalDeviceMemoryProperties");

    if (!pfn_vkCreateInstance || !pfn_vkDestroyInstance
        || !pfn_vkEnumeratePhysicalDevices || !pfn_vkGetPhysicalDeviceProperties
        || !pfn_vkGetPhysicalDeviceMemoryProperties) {
        SENTRY_DEBUG("Failed to load required Vulkan functions");
        SENTRY_FREE_LIBRARY(vulkan_library);
        vulkan_library = NULL;
        return false;
    }

    SENTRY_DEBUG("Successfully loaded Vulkan library and functions");
    return true;
}

static void
unload_vulkan_library(void)
{
    if (vulkan_library != NULL) {
        SENTRY_FREE_LIBRARY(vulkan_library);
        vulkan_library = NULL;
        pfn_vkCreateInstance = NULL;
        pfn_vkDestroyInstance = NULL;
        pfn_vkEnumeratePhysicalDevices = NULL;
        pfn_vkGetPhysicalDeviceProperties = NULL;
        pfn_vkGetPhysicalDeviceMemoryProperties = NULL;
    }
}

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
    VkResult result = pfn_vkCreateInstance(&create_info, NULL, &instance);
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

    pfn_vkGetPhysicalDeviceProperties(device, &properties);
    pfn_vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

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

    uint64_t total_memory = 0;
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
    if (!load_vulkan_library()) {
        return NULL;
    }

    VkInstance instance = create_vulkan_instance();
    if (instance == VK_NULL_HANDLE) {
        unload_vulkan_library();
        return NULL;
    }

    uint32_t device_count = 0;
    VkResult result
        = pfn_vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (result != VK_SUCCESS || device_count == 0) {
        SENTRY_DEBUGF("Failed to enumerate Vulkan devices: %d", result);
        pfn_vkDestroyInstance(instance, NULL);
        unload_vulkan_library();
        return NULL;
    }

    VkPhysicalDevice *devices
        = sentry_malloc(sizeof(VkPhysicalDevice) * device_count);
    if (!devices) {
        pfn_vkDestroyInstance(instance, NULL);
        unload_vulkan_library();
        return NULL;
    }

    result = pfn_vkEnumeratePhysicalDevices(instance, &device_count, devices);
    if (result != VK_SUCCESS) {
        SENTRY_DEBUGF("Failed to get Vulkan physical devices: %d", result);
        sentry_free(devices);
        pfn_vkDestroyInstance(instance, NULL);
        unload_vulkan_library();
        return NULL;
    }

    sentry_gpu_list_t *gpu_list = sentry_malloc(sizeof(sentry_gpu_list_t));
    if (!gpu_list) {
        sentry_free(devices);
        pfn_vkDestroyInstance(instance, NULL);
        unload_vulkan_library();
        return NULL;
    }

    gpu_list->gpus = sentry_malloc(sizeof(sentry_gpu_info_t *) * device_count);
    if (!gpu_list->gpus) {
        sentry_free(gpu_list);
        sentry_free(devices);
        pfn_vkDestroyInstance(instance, NULL);
        unload_vulkan_library();
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
    pfn_vkDestroyInstance(instance, NULL);

    if (gpu_list->count == 0) {
        sentry_free(gpu_list->gpus);
        sentry_free(gpu_list);
        unload_vulkan_library();
        return NULL;
    }

    // Clean up the dynamically loaded Vulkan library since we're done with it
    unload_vulkan_library();

    return gpu_list;
}
