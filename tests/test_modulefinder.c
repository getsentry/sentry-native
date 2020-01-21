#include "../src/sentry_modulefinder.h"
#include "sentry_testsupport.h"
#include <sentry.h>

#ifdef SENTRY_PLATFORM_LINUX
#    include "../src/linux/sentry_procmaps_modulefinder.h"
#endif

SENTRY_TEST(test_module_finder)
{
    sentry_value_t modules = sentry__modules_get_list();
    TEST_CHECK(sentry_value_get_length(modules) > 0);

    bool found_test = false;
    for (size_t i = 0; i < sentry_value_get_length(modules); i++) {
        sentry_value_t mod = sentry_value_get_by_index(modules, i);
        sentry_value_t name = sentry_value_get_by_key(mod, "code_file");
        const char *name_str = sentry_value_as_string(name);
        if (strstr(name_str, "sentry_tests")) {
            // our tests should also have at least a debug_id on all platforms
            sentry_value_t debug_id = sentry_value_get_by_key(mod, "debug_id");
            TEST_CHECK(
                sentry_value_get_type(debug_id) == SENTRY_VALUE_TYPE_STRING);

            found_test = true;
        }
    }

    TEST_CHECK(found_test);
}

SENTRY_TEST(test_procmaps_parser)
{
#ifdef SENTRY_PLATFORM_LINUX
    sentry_module_t mod;
    char contents[]
        = "7fdb549ce000-7fdb54bb5000 r-xp 00000000 08:01 3803938       "
          "             /lib/x86_64-linux-gnu/libc-2.27.so\n"
          "7f14753de000-7f14755de000 ---p 001e7000 08:01 3803938       "
          "             /lib/x86_64-linux-gnu/libc-2.27.so\n"
          "7fe714493000-7fe714494000 rw-p 00000000 00:00 0\n"
          "7fff8ca67000-7fff8ca88000 rw-p 00000000 00:00 0             "
          "             [vdso]";
    char *lines = contents;
    int read;

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == 140579994132480);
    TEST_CHECK(mod.end == 140579996127232);
    TEST_CHECK_STRING_EQUAL(mod.file, "/lib/x86_64-linux-gnu/libc-2.27.so");
    sentry_free(mod.file);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == 139725843062784);
    TEST_CHECK(mod.end == 139725845159936);
    sentry_free(mod.file);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == 140630454513664);
    TEST_CHECK(mod.end == 140630454517760);
    TEST_CHECK(mod.file == NULL);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == 140735553105920);
    TEST_CHECK(mod.end == 140735553241088);
    TEST_CHECK_STRING_EQUAL(mod.file, "[vdso]");
    sentry_free(mod.file);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    TEST_CHECK(!read);
#endif
}
