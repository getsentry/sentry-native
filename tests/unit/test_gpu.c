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
    // Test the common vendor ID to name mapping function with all supported
    // vendors
    unsigned int test_vendor_ids[]
        = { 0x10DE, 0x1002, 0x8086, 0x106B, 0x1414, 0x5143, 0x1AE0, 0x1010,
              0x1023, 0x102B, 0x121A, 0x18CA, 0x1039, 0x126F, 0x0000, 0xFFFF };

    for (size_t i = 0; i < sizeof(test_vendor_ids) / sizeof(test_vendor_ids[0]);
        i++) {
        char *vendor_name = sentry__gpu_vendor_id_to_name(test_vendor_ids[i]);
        TEST_CHECK(vendor_name != NULL);

        switch (test_vendor_ids[i]) {
        case 0x10DE:
            TEST_CHECK(strstr(vendor_name, "NVIDIA") != NULL);
            break;
        case 0x1002:
            TEST_CHECK(strstr(vendor_name, "AMD") != NULL
                || strstr(vendor_name, "ATI") != NULL);
            break;
        case 0x8086:
            TEST_CHECK(strstr(vendor_name, "Intel") != NULL);
            break;
        case 0x106B:
            TEST_CHECK(strstr(vendor_name, "Apple") != NULL);
            break;
        case 0x1414:
            TEST_CHECK(strstr(vendor_name, "Microsoft") != NULL);
            break;
        case 0x5143:
            TEST_CHECK(strstr(vendor_name, "Qualcomm") != NULL);
            break;
        case 0x1AE0:
            TEST_CHECK(strstr(vendor_name, "Google") != NULL);
            break;
        case 0x1010:
            TEST_CHECK(strstr(vendor_name, "VideoLogic") != NULL);
            break;
        case 0x1023:
            TEST_CHECK(strstr(vendor_name, "Trident") != NULL);
            break;
        case 0x102B:
            TEST_CHECK(strstr(vendor_name, "Matrox") != NULL);
            break;
        case 0x121A:
            TEST_CHECK(strstr(vendor_name, "3dfx") != NULL);
            break;
        case 0x18CA:
            TEST_CHECK(strstr(vendor_name, "XGI") != NULL);
            break;
        case 0x1039:
            TEST_CHECK(strstr(vendor_name, "SiS") != NULL
                || strstr(vendor_name, "Silicon") != NULL);
            break;
        case 0x126F:
            TEST_CHECK(strstr(vendor_name, "Silicon Motion") != NULL);
            break;
        case 0x0000:
        case 0xFFFF:
            TEST_CHECK(strstr(vendor_name, "Unknown") != NULL);
            TEST_CHECK(strstr(vendor_name, "0x") != NULL);
            break;
        default:
            TEST_CHECK(strstr(vendor_name, "Unknown") != NULL);
            TEST_CHECK(strstr(vendor_name, "0x") != NULL);
            break;
        }

        sentry_free(vendor_name);
    }

    // Test with actual GPU info if available
    if (gpu_info) {
        if (gpu_info->vendor_name) {
            char *expected_vendor_name
                = sentry__gpu_vendor_id_to_name(gpu_info->vendor_id);
            TEST_CHECK(expected_vendor_name != NULL);

            if (expected_vendor_name) {
                // Use strstr to check that the vendor name contains expected
                // content rather than exact string comparison which may be
                // fragile
                switch (gpu_info->vendor_id) {
                case 0x10DE: // NVIDIA
                    TEST_CHECK(strstr(gpu_info->vendor_name, "NVIDIA") != NULL);
                    break;
                case 0x1002: // AMD/ATI
                    TEST_CHECK(strstr(gpu_info->vendor_name, "AMD") != NULL
                        || strstr(gpu_info->vendor_name, "ATI") != NULL);
                    break;
                case 0x8086: // Intel
                    TEST_CHECK(strstr(gpu_info->vendor_name, "Intel") != NULL);
                    break;
                case 0x106B: // Apple
                    TEST_CHECK(strstr(gpu_info->vendor_name, "Apple") != NULL);
                    break;
                case 0x1414: // Microsoft
                    TEST_CHECK(
                        strstr(gpu_info->vendor_name, "Microsoft") != NULL);
                    break;
                default:
                    // For other or unknown vendors, just check it's not empty
                    TEST_CHECK(strlen(gpu_info->vendor_name) > 0);
                    break;
                }

                sentry_free(expected_vendor_name);
            }
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

        // Free the GPU context
        sentry_value_decref(gpu_context);
    } else {
        TEST_MSG("No GPU context available on this system");
    }
#else
    // When GPU support is disabled, should always get null
    TEST_CHECK(sentry_value_is_null(gpu_context));
    TEST_MSG("GPU support disabled - correctly returned null context");
#endif
}
