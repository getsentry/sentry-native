#include "sentry_modulefinder.h"
#include "sentry_path.h"
#include "sentry_testsupport.h"
#include <sentry.h>

#ifdef SENTRY_PLATFORM_LINUX
#    include "modulefinder/sentry_modulefinder_linux.h"
#endif

SENTRY_TEST(module_finder)
{
    sentry_value_t modules = sentry__modules_get_list();
    TEST_CHECK(sentry_value_get_length(modules) > 0);

    bool found_test = false;
    for (size_t i = 0; i < sentry_value_get_length(modules); i++) {
        sentry_value_t mod = sentry_value_get_by_index(modules, i);
        sentry_value_t name = sentry_value_get_by_key(mod, "code_file");
        const char *name_str = sentry_value_as_string(name);
        if (strstr(name_str, "sentry_test_unit")) {
            // our tests should also have at least a debug_id on all platforms
            sentry_value_t debug_id = sentry_value_get_by_key(mod, "debug_id");
            TEST_CHECK(
                sentry_value_get_type(debug_id) == SENTRY_VALUE_TYPE_STRING);

            found_test = true;
        }
    }

    TEST_CHECK(found_test);
}

SENTRY_TEST(procmaps_parser)
{
#if !defined(SENTRY_PLATFORM_LINUX) || __SIZEOF_POINTER__ != 8
    SKIP_TEST();
#else
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

SENTRY_TEST(buildid_fallback)
{
#ifndef SENTRY_PLATFORM_LINUX
    SKIP_TEST();
#else
    sentry_path_t *path = sentry__path_new(__FILE__);
    sentry_path_t *dir = sentry__path_dir(path);
    sentry__path_free(path);

    sentry_path_t *with_id_path
        = sentry__path_join_str(dir, "../fixtures/with-buildid.so");
    size_t with_id_len = 0;
    char *with_id = sentry__path_read_to_buffer(with_id_path, &with_id_len);
    sentry__path_free(with_id_path);

    sentry_module_t with_id_mod
        = { with_id, with_id + with_id_len, { "with-buildid.so", 15 } };
    sentry_value_t with_id_val = sentry__procmaps_module_to_value(&with_id_mod);

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(with_id_val, "code_id")),
        "1c304742f114215453a8a777f6cdb3a2b8505e11");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                with_id_val, "debug_id")),
        "4247301c-14f1-5421-53a8-a777f6cdb3a2");
    sentry_value_decref(with_id_val);

    sentry_path_t *without_id_path
        = sentry__path_join_str(dir, "../fixtures/without-buildid.so");
    size_t without_id_len = 0;
    char *without_id
        = sentry__path_read_to_buffer(without_id_path, &without_id_len);
    sentry__path_free(without_id_path);

    sentry_module_t without_id_mod = { without_id, without_id + without_id_len,
        { "without-buildid.so", 18 } };
    sentry_value_t without_id_val
        = sentry__procmaps_module_to_value(&without_id_mod);

    TEST_CHECK(sentry_value_is_null(
        sentry_value_get_by_key(without_id_val, "code_id")));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                without_id_val, "debug_id")),
        "29271919-a2ef-129d-9aac-be85a0948d9c");
    sentry_value_decref(without_id_val);

    sentry_free(with_id);
    sentry_free(without_id);

    sentry__path_free(dir);
#endif
}
