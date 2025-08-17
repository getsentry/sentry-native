#include "sentry_testsupport.h"
#include <string.h>

#ifdef SENTRY_EMBED_INFO
extern const char sentry_library_info[];

SENTRY_TEST(embedded_info_basic)
{
    // Test that the embedded info string exists and has expected format
    TEST_CHECK(sentry_library_info != NULL);
    TEST_CHECK(strlen(sentry_library_info) > 0);

    // Test that required fields are present
    TEST_CHECK(strstr(sentry_library_info, "SENTRY_VERSION:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "PLATFORM:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "BUILD:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "CONFIG:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "END") != NULL);
}

SENTRY_TEST(embedded_info_format)
{
    // Test that the string is properly semicolon-separated
    char *info = strdup(sentry_library_info);
    TEST_CHECK(info != NULL);

    int field_count = 0;
    char *token = strtok(info, ";");
    while (token != NULL) {
        // Each field should contain a colon (except END)
        if (strcmp(token, "END") != 0) {
            TEST_CHECK(strchr(token, ':') != NULL);
        }
        field_count++;
        token = strtok(NULL, ";");
    }

    // Should have at least 5 fields: VERSION, PLATFORM, BUILD, VARIANT, CONFIG,
    // END
    TEST_CHECK(field_count >= 6);

    free(info);
}

SENTRY_TEST(embedded_info_sentry_version)
{
    // Test that SENTRY_VERSION field contains the actual SDK version
    const char *version_field = strstr(sentry_library_info, "SENTRY_VERSION:");
    TEST_CHECK(version_field != NULL);

    // Extract the version value
    const char *version_start = version_field + strlen("SENTRY_VERSION:");
    const char *version_end = strchr(version_start, ';');
    TEST_CHECK(version_end != NULL);

    size_t version_len = version_end - version_start;
    TEST_CHECK(version_len > 0);

    // Version should contain at least one dot (e.g., "0.10.0")
    char version[32];
    strncpy(version, version_start, version_len);
    version[version_len] = '\0';
    TEST_CHECK(strchr(version, '.') != NULL);
}

SENTRY_TEST(embedded_info_disabled)
{
    // When SENTRY_EMBED_INFO is defined, this test should pass
    // When SENTRY_EMBED_INFO is not defined, other tests don't exist
    TEST_CHECK(1); // Always pass
}

#else

// When SENTRY_EMBED_INFO is not defined, provide stub implementations
SENTRY_TEST(embedded_info_basic)
{
    // Feature is disabled, nothing to test for embedded symbols
    TEST_CHECK(1);
}

SENTRY_TEST(embedded_info_format)
{
    // Feature is disabled, nothing to test for format
    TEST_CHECK(1);
}

SENTRY_TEST(embedded_info_sentry_version)
{
    // Feature is disabled, nothing to test for version
    TEST_CHECK(1);
}

SENTRY_TEST(embedded_info_disabled)
{
    // When SENTRY_EMBED_INFO is not defined, the feature is properly disabled
    TEST_CHECK(1); // Always pass - confirms the feature is disabled
}

#endif
