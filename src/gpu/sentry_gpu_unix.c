#include "sentry_gpu.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef SENTRY_PLATFORM_LINUX
#    include <dirent.h>
#    include <sys/stat.h>
#endif

#ifdef SENTRY_PLATFORM_MACOS
#    include <CoreFoundation/CoreFoundation.h>
#    include <IOKit/IOKitLib.h>
#    include <IOKit/graphics/IOGraphicsLib.h>
#    include <sys/sysctl.h>
#endif

#ifdef SENTRY_PLATFORM_LINUX
static char *
read_file_content(const char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length <= 0) {
        fclose(file);
        return NULL;
    }

    char *content = sentry_malloc(length + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, 1, length, file);
    fclose(file);

    content[read_size] = '\0';

    char *newline = strchr(content, '\n');
    if (newline) {
        *newline = '\0';
    }

    return content;
}

static unsigned int
parse_hex_id(const char *hex_str)
{
    if (!hex_str) {
        return 0;
    }

    char *prefixed_str = NULL;
    if (strncmp(hex_str, "0x", 2) != 0) {
        size_t len = strlen(hex_str) + 3;
        prefixed_str = sentry_malloc(len);
        if (prefixed_str) {
            snprintf(prefixed_str, len, "0x%s", hex_str);
        }
    }

    unsigned int result = (unsigned int)strtoul(
        prefixed_str ? prefixed_str : hex_str, NULL, 16);

    if (prefixed_str) {
        sentry_free(prefixed_str);
    }

    return result;
}
static sentry_gpu_info_t *
get_gpu_info_linux_pci(void)
{
    DIR *pci_dir = opendir("/sys/bus/pci/devices");
    if (!pci_dir) {
        return NULL;
    }

    sentry_gpu_info_t *gpu_info = NULL;
    struct dirent *entry;

    while ((entry = readdir(pci_dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char class_path[512];
        snprintf(class_path, sizeof(class_path),
            "/sys/bus/pci/devices/%s/class", entry->d_name);

        char *class_str = read_file_content(class_path);
        if (!class_str) {
            continue;
        }

        unsigned int class_code = parse_hex_id(class_str);
        sentry_free(class_str);

        if ((class_code >> 16) != 0x03) {
            continue;
        }

        gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
        if (!gpu_info) {
            break;
        }

        memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

        char vendor_path[512], device_path[512];
        snprintf(vendor_path, sizeof(vendor_path),
            "/sys/bus/pci/devices/%s/vendor", entry->d_name);
        snprintf(device_path, sizeof(device_path),
            "/sys/bus/pci/devices/%s/device", entry->d_name);

        char *vendor_str = read_file_content(vendor_path);
        char *device_str = read_file_content(device_path);

        if (vendor_str) {
            gpu_info->vendor_id = parse_hex_id(vendor_str);
            sentry_free(vendor_str);
        }

        if (device_str) {
            gpu_info->device_id = parse_hex_id(device_str);
            sentry_free(device_str);
        }

        gpu_info->vendor_name
            = sentry__gpu_vendor_id_to_name(gpu_info->vendor_id);

        break;
    }

    closedir(pci_dir);
    return gpu_info;
}

static sentry_gpu_info_t *
get_gpu_info_linux_drm(void)
{
    DIR *drm_dir = opendir("/sys/class/drm");
    if (!drm_dir) {
        return NULL;
    }

    sentry_gpu_info_t *gpu_info = NULL;
    struct dirent *entry;

    while ((entry = readdir(drm_dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) != 0) {
            continue;
        }

        char name_path[512];
        snprintf(name_path, sizeof(name_path),
            "/sys/class/drm/%s/device/driver", entry->d_name);

        char link_target[512];
        ssize_t len = readlink(name_path, link_target, sizeof(link_target) - 1);
        if (len > 0) {
            link_target[len] = '\0';
            char *driver_name = strrchr(link_target, '/');
            if (driver_name) {
                gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
                if (gpu_info) {
                    memset(gpu_info, 0, sizeof(sentry_gpu_info_t));
                    gpu_info->name = sentry__string_clone(driver_name + 1);
                }
                break;
            }
        }
    }

    closedir(drm_dir);
    return gpu_info;
}
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
get_gpu_info_macos_pci(void)
{
    sentry_gpu_info_t *gpu_info = NULL;
    io_iterator_t iterator = IO_OBJECT_NULL;

    mach_port_t main_port;
#    if defined(MAC_OS_VERSION_12_0)                                           \
        && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_0
    main_port = kIOMainPortDefault;
#    else
    main_port = kIOMasterPortDefault;
#    endif

    kern_return_t result = IOServiceGetMatchingServices(
        main_port, IOServiceMatching("IOPCIDevice"), &iterator);

    if (result != KERN_SUCCESS) {
        return NULL;
    }

    io_object_t service;
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        CFMutableDictionaryRef properties = NULL;
        result = IORegistryEntryCreateCFProperties(
            service, &properties, kCFAllocatorDefault, kNilOptions);

        if (result == KERN_SUCCESS && properties) {
            CFNumberRef class_code_ref
                = CFDictionaryGetValue(properties, CFSTR("class-code"));
            if (class_code_ref
                && CFGetTypeID(class_code_ref) == CFNumberGetTypeID()) {
                uint32_t class_code = 0;
                CFNumberGetValue(
                    class_code_ref, kCFNumberSInt32Type, &class_code);

                if ((class_code >> 16) == 0x03) {
                    gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
                    if (gpu_info) {
                        memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

                        CFNumberRef vendor_id_ref = CFDictionaryGetValue(
                            properties, CFSTR("vendor-id"));
                        if (vendor_id_ref
                            && CFGetTypeID(vendor_id_ref)
                                == CFNumberGetTypeID()) {
                            uint32_t vendor_id = 0;
                            CFNumberGetValue(
                                vendor_id_ref, kCFNumberSInt32Type, &vendor_id);
                            gpu_info->vendor_id = vendor_id;
                        }

                        CFNumberRef device_id_ref = CFDictionaryGetValue(
                            properties, CFSTR("device-id"));
                        if (device_id_ref
                            && CFGetTypeID(device_id_ref)
                                == CFNumberGetTypeID()) {
                            uint32_t device_id = 0;
                            CFNumberGetValue(
                                device_id_ref, kCFNumberSInt32Type, &device_id);
                            gpu_info->device_id = device_id;
                        }

                        CFStringRef model_ref
                            = CFDictionaryGetValue(properties, CFSTR("model"));
                        if (model_ref
                            && CFGetTypeID(model_ref) == CFStringGetTypeID()) {
                            CFIndex length = CFStringGetLength(model_ref);
                            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(
                                                  length, kCFStringEncodingUTF8)
                                + 1;
                            char *model_str = sentry_malloc(maxSize);
                            if (model_str
                                && CFStringGetCString(model_ref, model_str,
                                    maxSize, kCFStringEncodingUTF8)) {
                                gpu_info->name = model_str;
                            } else {
                                sentry_free(model_str);
                            }
                        }

                        gpu_info->vendor_name = sentry__gpu_vendor_id_to_name(
                            gpu_info->vendor_id);
                    }

                    CFRelease(properties);
                    IOObjectRelease(service);
                    break;
                }
            }

            CFRelease(properties);
        }

        IOObjectRelease(service);
    }

    IOObjectRelease(iterator);
    return gpu_info;
}

static sentry_gpu_info_t *
get_gpu_info_macos(void)
{
    sentry_gpu_info_t *gpu_info = NULL;

    // Try Apple Silicon GPU first
    gpu_info = get_gpu_info_macos_agx();
    if (gpu_info) {
        return gpu_info;
    }

    // Fallback to PCI-based GPUs (Intel Macs, eGPUs, etc.)
    gpu_info = get_gpu_info_macos_pci();
    return gpu_info;
}
#endif

sentry_gpu_info_t *
sentry__get_gpu_info(void)
{
    sentry_gpu_info_t *gpu_info = NULL;

#ifdef SENTRY_PLATFORM_LINUX
    gpu_info = get_gpu_info_linux_pci();
    if (!gpu_info) {
        gpu_info = get_gpu_info_linux_drm();
    }
#endif

#ifdef SENTRY_PLATFORM_MACOS
    gpu_info = get_gpu_info_macos();
#endif

    return gpu_info;
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
