#include "sentry_gpu.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"

SENTRY_TEST(gpu_info_basic)
{
    sentry_gpu_list_t *gpu_list = sentry__get_gpu_info();

#ifdef SENTRY_WITH_GPU_INFO
    // When GPU support is enabled, we should get some GPU information (at least
    // on most systems)
    if (gpu_list && gpu_list->count > 0) {
        printf("Found %u GPU(s):\n", gpu_list->count);

        // Check that at least one GPU has populated fields
        bool has_info = false;
        for (unsigned int i = 0; i < gpu_list->count; i++) {
            sentry_gpu_info_t *gpu_info = gpu_list->gpus[i];
            printf("GPU %u:\n", i);

            if (gpu_info->name && strlen(gpu_info->name) > 0) {
                has_info = true;
                printf("  Name: %s\n", gpu_info->name);
            }
            if (gpu_info->vendor_name && strlen(gpu_info->vendor_name) > 0) {
                has_info = true;
                printf("  Vendor: %s\n", gpu_info->vendor_name);
            }
            if (gpu_info->vendor_id != 0) {
                has_info = true;
                printf("  Vendor ID: 0x%04X\n", gpu_info->vendor_id);
            }
            if (gpu_info->device_id != 0) {
                has_info = true;
                printf("  Device ID: 0x%04X\n", gpu_info->device_id);
            }
            if (gpu_info->driver_version
                && strlen(gpu_info->driver_version) > 0) {
                has_info = true;
                printf("  Driver Version: %s\n", gpu_info->driver_version);
            }
            if (gpu_info->memory_size > 0) {
                has_info = true;
                printf("  Memory Size: %zu bytes\n", gpu_info->memory_size);
            }
        }

        TEST_CHECK(has_info);
        TEST_MSG("At least one GPU info field should be populated");

        sentry__free_gpu_list(gpu_list);
    } else {
        // It's okay if no GPU info is available on some systems (VMs, headless
        // systems, etc.)
        TEST_MSG("No GPU information available on this system");
    }
#else
    // When GPU support is disabled, we should always get NULL
    TEST_CHECK(gpu_list == NULL);
    TEST_MSG("GPU support disabled - correctly returned NULL");
#endif
}

SENTRY_TEST(gpu_info_free_null)
{
    // Test that freeing NULL doesn't crash
    sentry__free_gpu_info(NULL);
    sentry__free_gpu_list(NULL);
    TEST_CHECK(1); // If we get here, the test passed
}

SENTRY_TEST(gpu_info_vendor_id_known)
{
    sentry_gpu_list_t *gpu_list = sentry__get_gpu_info();

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
    if (gpu_list && gpu_list->count > 0) {
        for (unsigned int i = 0; i < gpu_list->count; i++) {
            sentry_gpu_info_t *gpu_info = gpu_list->gpus[i];

            if (gpu_info->vendor_name) {
                char *expected_vendor_name
                    = sentry__gpu_vendor_id_to_name(gpu_info->vendor_id);
                TEST_CHECK(expected_vendor_name != NULL);

                if (expected_vendor_name) {
                    // Use strstr to check that the vendor name contains
                    // expected content rather than exact string comparison
                    // which may be fragile
                    switch (gpu_info->vendor_id) {
                    case 0x10DE: // NVIDIA
                        TEST_CHECK(
                            strstr(gpu_info->vendor_name, "NVIDIA") != NULL);
                        break;
                    case 0x1002: // AMD/ATI
                        TEST_CHECK(strstr(gpu_info->vendor_name, "AMD") != NULL
                            || strstr(gpu_info->vendor_name, "ATI") != NULL);
                        break;
                    case 0x8086: // Intel
                        TEST_CHECK(
                            strstr(gpu_info->vendor_name, "Intel") != NULL);
                        break;
                    case 0x106B: // Apple
                        TEST_CHECK(
                            strstr(gpu_info->vendor_name, "Apple") != NULL);
                        break;
                    case 0x1414: // Microsoft
                        TEST_CHECK(
                            strstr(gpu_info->vendor_name, "Microsoft") != NULL);
                        break;
                    default:
                        // For other or unknown vendors, just check it's not
                        // empty
                        TEST_CHECK(strlen(gpu_info->vendor_name) > 0);
                        break;
                    }

                    sentry_free(expected_vendor_name);
                }
            }
        }

        sentry__free_gpu_list(gpu_list);
    } else {
        TEST_MSG("No GPU vendor ID available for testing");
    }
#else
    // When GPU support is disabled, should return NULL
    TEST_CHECK(gpu_list == NULL);
    TEST_MSG("GPU support disabled - correctly returned NULL");
#endif
}

SENTRY_TEST(gpu_info_memory_allocation)
{
    // Test multiple allocations and frees
    for (int i = 0; i < 5; i++) {
        sentry_gpu_list_t *gpu_list = sentry__get_gpu_info();
#ifdef SENTRY_WITH_GPU_INFO
        if (gpu_list) {
            // Verify the structure is properly initialized
            TEST_CHECK(gpu_list != NULL);
            TEST_CHECK(gpu_list->count >= 0);
            if (gpu_list->count > 0) {
                TEST_CHECK(gpu_list->gpus != NULL);
            }
            sentry__free_gpu_list(gpu_list);
        }
#else
        // When GPU support is disabled, should always be NULL
        TEST_CHECK(gpu_list == NULL);
#endif
    }
    TEST_CHECK(1); // If we get here without crashing, test passed
}

SENTRY_TEST(gpu_context_scope_integration)
{
    // Test that GPU contexts are properly integrated into scope
    sentry_value_t contexts = sentry_value_new_object();
    TEST_CHECK(!sentry_value_is_null(contexts));

    sentry__add_gpu_contexts(contexts);

#ifdef SENTRY_WITH_GPU_INFO
    // When GPU support is enabled, check if we get valid contexts
    sentry_value_t gpu_context = sentry_value_get_by_key(contexts, "gpu");

    if (!sentry_value_is_null(gpu_context)) {
        // GPU context should be an object with type "gpu"
        TEST_CHECK(
            sentry_value_get_type(gpu_context) == SENTRY_VALUE_TYPE_OBJECT);

        // Check that type field is set to "gpu"
        sentry_value_t type_field
            = sentry_value_get_by_key(gpu_context, "type");
        TEST_CHECK(!sentry_value_is_null(type_field));
        TEST_CHECK(
            sentry_value_get_type(type_field) == SENTRY_VALUE_TYPE_STRING);

        const char *type_str = sentry_value_as_string(type_field);
        TEST_CHECK(type_str != NULL);
        TEST_CHECK(strcmp(type_str, "gpu") == 0);

        // Check that at least one GPU has valid fields
        sentry_value_t name = sentry_value_get_by_key(gpu_context, "name");
        sentry_value_t vendor_name
            = sentry_value_get_by_key(gpu_context, "vendor_name");
        sentry_value_t vendor_id
            = sentry_value_get_by_key(gpu_context, "vendor_id");

        bool has_field = !sentry_value_is_null(name)
            || !sentry_value_is_null(vendor_name)
            || !sentry_value_is_null(vendor_id);
        TEST_CHECK(has_field);
        TEST_MSG("Primary GPU should contain valid fields");

        // Check for additional GPUs (gpu2, gpu3, etc.)
        for (int i = 2; i <= 4; i++) {
            char context_key[16];
            snprintf(context_key, sizeof(context_key), "gpu%d", i);
            sentry_value_t additional_gpu
                = sentry_value_get_by_key(contexts, context_key);

            if (!sentry_value_is_null(additional_gpu)) {
                printf("Found additional GPU context: %s\n", context_key);

                // Check type field
                sentry_value_t type_field
                    = sentry_value_get_by_key(additional_gpu, "type");
                TEST_CHECK(!sentry_value_is_null(type_field));
                const char *type_str = sentry_value_as_string(type_field);
                TEST_CHECK(type_str != NULL);
                TEST_CHECK(strcmp(type_str, "gpu") == 0);
            }
        }
    } else {
        TEST_MSG("No GPU context available on this system");
    }
#else
    // When GPU support is disabled, should not have gpu context
    sentry_value_t gpu_context = sentry_value_get_by_key(contexts, "gpu");
    TEST_CHECK(sentry_value_is_null(gpu_context));
    TEST_MSG("GPU support disabled - correctly no GPU context");
#endif

    sentry_value_decref(contexts);
}

SENTRY_TEST(gpu_info_multi_gpu_support)
{
    sentry_gpu_list_t *gpu_list = sentry__get_gpu_info();

#ifdef SENTRY_WITH_GPU_INFO
    if (gpu_list && gpu_list->count > 0) {
        printf("Testing multi-GPU support with %u GPU(s)\n", gpu_list->count);

        // Test that all GPUs in the list are properly initialized
        for (unsigned int i = 0; i < gpu_list->count; i++) {
            sentry_gpu_info_t *gpu_info = gpu_list->gpus[i];
            TEST_CHECK(gpu_info != NULL);

            // At least vendor_id should be set for each GPU
            if (gpu_info->vendor_id == 0
                && (!gpu_info->name || strlen(gpu_info->name) == 0)) {
                TEST_MSG("GPU entry has no identifying information");
            }

            printf("GPU %u: vendor_id=0x%04X, name=%s\n", i,
                gpu_info->vendor_id,
                gpu_info->name ? gpu_info->name : "(null)");
        }

        // Test that we don't have duplicate pointers in the array
        if (gpu_list->count > 1) {
            for (unsigned int i = 0; i < gpu_list->count - 1; i++) {
                for (unsigned int j = i + 1; j < gpu_list->count; j++) {
                    TEST_CHECK(gpu_list->gpus[i] != gpu_list->gpus[j]);
                }
            }
        }

        sentry__free_gpu_list(gpu_list);
    } else {
        TEST_MSG("No multi-GPU setup detected - this is normal");
    }
#else
    TEST_CHECK(gpu_list == NULL);
    TEST_MSG("GPU support disabled - correctly returned NULL");
#endif
}

SENTRY_TEST(gpu_info_hybrid_setup_simulation)
{
    // This test simulates what should happen in a hybrid GPU setup
    sentry_gpu_list_t *gpu_list = sentry__get_gpu_info();

#ifdef SENTRY_WITH_GPU_INFO
    if (gpu_list && gpu_list->count > 1) {
        printf("Hybrid GPU setup detected with %u GPUs\n", gpu_list->count);

        bool has_nvidia = false;
        bool has_other = false;

        for (unsigned int i = 0; i < gpu_list->count; i++) {
            sentry_gpu_info_t *gpu_info = gpu_list->gpus[i];

            if (gpu_info->vendor_id == 0x10de) { // NVIDIA
                has_nvidia = true;
                printf("Found NVIDIA GPU: %s\n",
                    gpu_info->name ? gpu_info->name : "Unknown");

                // NVIDIA GPUs should have more detailed info if NVML worked
                if (gpu_info->driver_version) {
                    printf("  Driver: %s\n", gpu_info->driver_version);
                }
                if (gpu_info->memory_size > 0) {
                    printf("  Memory: %zu bytes\n", gpu_info->memory_size);
                }
            } else {
                has_other = true;
                printf("Found other GPU: vendor=0x%04X, name=%s\n",
                    gpu_info->vendor_id,
                    gpu_info->name ? gpu_info->name : "Unknown");
            }
        }

        if (has_nvidia && has_other) {
            TEST_MSG("Successfully detected hybrid NVIDIA + other GPU setup");
        }

        sentry__free_gpu_list(gpu_list);
    } else {
        TEST_MSG("No hybrid GPU setup detected - this is normal");
    }
#else
    TEST_CHECK(gpu_list == NULL);
    TEST_MSG("GPU support disabled");
#endif
}
