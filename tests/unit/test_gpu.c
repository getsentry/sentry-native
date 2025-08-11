#include "sentry_gpu.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"

SENTRY_TEST(gpu_info_basic)
{
    sentry_gpu_info_t *gpu_info = sentry__get_gpu_info();

#ifdef SENTRY_WITH_GPU_INFO
    // When GPU support is enabled, we should get some GPU information (at least
    // on most systems)
    if (gpu_info) {
        // Check that at least one field is populated
        bool has_info = false;
        if (gpu_info->name && strlen(gpu_info->name) > 0) {
            has_info = true;
            printf("GPU Name: %s\n", gpu_info->name);
        }
        if (gpu_info->vendor_name && strlen(gpu_info->vendor_name) > 0) {
            has_info = true;
            printf("Vendor: %s\n", gpu_info->vendor_name);
        }
        if (gpu_info->vendor_id != 0) {
            has_info = true;
            printf("Vendor ID: 0x%04X\n", gpu_info->vendor_id);
        }
        if (gpu_info->device_id != 0) {
            has_info = true;
            printf("Device ID: 0x%04X\n", gpu_info->device_id);
        }
        if (gpu_info->driver_version && strlen(gpu_info->driver_version) > 0) {
            has_info = true;
            printf("Driver Version: %s\n", gpu_info->driver_version);
        }
        if (gpu_info->memory_size > 0) {
            has_info = true;
            printf("Memory Size: %zu bytes\n", gpu_info->memory_size);
        }

        TEST_CHECK(has_info);
        TEST_MSG("At least one GPU info field should be populated");

        sentry__free_gpu_info(gpu_info);
    } else {
        // It's okay if no GPU info is available on some systems (VMs, headless
        // systems, etc.)
        TEST_MSG("No GPU information available on this system");
    }
#else
    // When GPU support is disabled, we should always get NULL
    TEST_CHECK(gpu_info == NULL);
    TEST_MSG("GPU support disabled - correctly returned NULL");
#endif
}

SENTRY_TEST(gpu_info_free_null)
{
    // Test that freeing NULL doesn't crash
    sentry__free_gpu_info(NULL);
    TEST_CHECK(1); // If we get here, the test passed
}

SENTRY_TEST(gpu_info_vendor_id_known)
{
    sentry_gpu_info_t *gpu_info = sentry__get_gpu_info();

#ifdef SENTRY_WITH_GPU_INFO
    if (gpu_info && gpu_info->vendor_id != 0) {
        // Test known vendor IDs
        switch (gpu_info->vendor_id) {
        case 0x10DE: // NVIDIA
            TEST_CHECK(gpu_info->vendor_name != NULL);
            TEST_CHECK(strstr(gpu_info->vendor_name, "NVIDIA") != NULL);
            break;
        case 0x1002: // AMD/ATI
            TEST_CHECK(gpu_info->vendor_name != NULL);
            TEST_CHECK(strstr(gpu_info->vendor_name, "AMD") != NULL
                || strstr(gpu_info->vendor_name, "ATI") != NULL);
            break;
        case 0x8086: // Intel
            TEST_CHECK(gpu_info->vendor_name != NULL);
            TEST_CHECK(strstr(gpu_info->vendor_name, "Intel") != NULL);
            break;
        case 0x106B: // Apple (macOS only)
            TEST_CHECK(gpu_info->vendor_name != NULL);
            TEST_CHECK(strstr(gpu_info->vendor_name, "Apple") != NULL);
            break;
        case 0x1414: // Microsoft (Windows only)
            TEST_CHECK(gpu_info->vendor_name != NULL);
            TEST_CHECK(strstr(gpu_info->vendor_name, "Microsoft") != NULL);
            break;
        default:
            // Unknown vendor should have "Unknown" name
            TEST_CHECK(gpu_info->vendor_name != NULL);
            TEST_CHECK(strcmp(gpu_info->vendor_name, "Unknown") == 0);
            break;
        }

        sentry__free_gpu_info(gpu_info);
    } else {
        TEST_MSG("No GPU vendor ID available for testing");
    }
#else
    // When GPU support is disabled, should return NULL
    TEST_CHECK(gpu_info == NULL);
    TEST_MSG("GPU support disabled - correctly returned NULL");
#endif
}

SENTRY_TEST(gpu_info_memory_allocation)
{
    // Test multiple allocations and frees
    for (int i = 0; i < 5; i++) {
        sentry_gpu_info_t *gpu_info = sentry__get_gpu_info();
#ifdef SENTRY_WITH_GPU_INFO
        if (gpu_info) {
            // Verify the structure is properly initialized
            TEST_CHECK(gpu_info != NULL);
            sentry__free_gpu_info(gpu_info);
        }
#else
        // When GPU support is disabled, should always be NULL
        TEST_CHECK(gpu_info == NULL);
#endif
    }
    TEST_CHECK(1); // If we get here without crashing, test passed
}

SENTRY_TEST(gpu_context_scope_integration)
{
    // Test that GPU context is properly integrated into scope
    sentry_value_t gpu_context = sentry__get_gpu_context();

#ifdef SENTRY_WITH_GPU_INFO
    // When GPU support is enabled, check if we get a valid context
    if (!sentry_value_is_null(gpu_context)) {
        TEST_CHECK(
            sentry_value_get_type(gpu_context) == SENTRY_VALUE_TYPE_OBJECT);

        // Check that at least one field is present in the context
        bool has_field = false;
        sentry_value_t name = sentry_value_get_by_key(gpu_context, "name");
        sentry_value_t vendor_name
            = sentry_value_get_by_key(gpu_context, "vendor_name");
        sentry_value_t vendor_id
            = sentry_value_get_by_key(gpu_context, "vendor_id");

        if (!sentry_value_is_null(name) || !sentry_value_is_null(vendor_name)
            || !sentry_value_is_null(vendor_id)) {
            has_field = true;
        }

        TEST_CHECK(has_field);
        TEST_MSG("GPU context should contain at least one valid field");
    } else {
        TEST_MSG("No GPU context available on this system");
    }
#else
    // When GPU support is disabled, should always get null
    TEST_CHECK(sentry_value_is_null(gpu_context));
    TEST_MSG("GPU support disabled - correctly returned null context");
#endif
}
