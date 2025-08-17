#include "sentry_testsupport.h"
#include <string.h>

#ifdef SENTRY_EMBED_INFO
extern const char sentry_library_info[];
#endif

SENTRY_TEST(embedded_info_basic)
{
#ifdef SENTRY_EMBED_INFO
    // Test that the embedded info string exists and has expected format
    TEST_CHECK(sentry_library_info != NULL);
    TEST_CHECK(strlen(sentry_library_info) > 0);

    // Test that required fields are present
    TEST_CHECK(strstr(sentry_library_info, "SENTRY_VERSION:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "PLATFORM:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "BUILD:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "CONFIG:") != NULL);
    TEST_CHECK(strstr(sentry_library_info, "END") != NULL);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(embedded_info_format)
{
#ifdef SENTRY_EMBED_INFO
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
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(embedded_info_sentry_version)
{
#ifdef SENTRY_EMBED_INFO
    // Test that SENTRY_VERSION field contains the actual SDK version
    const char *version_field = strstr(sentry_library_info, "SENTRY_VERSION:");
    TEST_CHECK(version_field != NULL);

    // Extract the version value
    const char *version_start = version_field + strlen("SENTRY_VERSION:");
    const char *version_end = strchr(version_start, ';');
    TEST_CHECK(version_end != NULL);

    size_t version_len = version_end - version_start;
    TEST_CHECK(version_len > 0);

    // Extract the embedded version string
    char embedded_version[32];
    strncpy(embedded_version, version_start, version_len);
    embedded_version[version_len] = '\0';

    // Version should contain at least one dot (e.g., "0.10.0")
    TEST_CHECK(strchr(embedded_version, '.') != NULL);

    // Test that it matches the actual SDK version
    TEST_CHECK_STRING_EQUAL(embedded_version, SENTRY_SDK_VERSION);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(embedded_info_disabled)
{
#ifndef SENTRY_EMBED_INFO
    // When SENTRY_EMBED_INFO is not defined, the feature is properly disabled
    TEST_CHECK(1); // Always pass - confirms the feature is disabled
#else
    SKIP_TEST(); // Skip when embedding is enabled
#endif
}
