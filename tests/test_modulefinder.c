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
    TEST_CHECK(mod.start == (void *)0x7fdb549ce000);
    TEST_CHECK(mod.end == (void *)0x7fdb54bb5000);
    TEST_CHECK(strncmp(mod.file.ptr, "/lib/x86_64-linux-gnu/libc-2.27.so",
                   mod.file.len)
        == 0);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == (void *)0x7f14753de000);
    TEST_CHECK(mod.end == (void *)0x7f14755de000);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == (void *)0x7fe714493000);
    TEST_CHECK(mod.end == (void *)0x7fe714494000);
    TEST_CHECK(mod.file.ptr == NULL);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    lines += read;
    TEST_CHECK(read);
    TEST_CHECK(mod.start == (void *)0x7fff8ca67000);
    TEST_CHECK(mod.end == (void *)0x7fff8ca88000);
    TEST_CHECK(strncmp(mod.file.ptr, "[vdso]", mod.file.len) == 0);

    read = sentry__procmaps_parse_module_line(lines, &mod);
    TEST_CHECK(!read);
#endif
}
