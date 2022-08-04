#include "sentry_boot.h"
#include "sentry_core.h"

#include <ucontext.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/system_properties.h>

typedef size_t (*android_unwind_stack_t)(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames);

static android_unwind_stack_t android_unwind_stack;
static void *handle;

size_t
sentry__unwind_stack_libunwindstack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (android_unwind_stack) {
        return android_unwind_stack(addr, uctx, ptrs, max_frames);
    } else {
        SENTRY_WARN("cannot unwind: unwinder not loaded");
        return 0;
    }
}

void
sentry__load_unwinder()
{
    char sdk_ver_str[16];
    int sdk_ver;
    if (__system_property_get("ro.build.version.sdk", sdk_ver_str)) {
        char *end;
        sdk_ver = strtol(sdk_ver_str, &end, 10);
        if (sdk_ver_str == end) {
            SENTRY_WARN(
                "failed to read android SDK version to choose unwinder");
            return;
        }
    } else {
        SENTRY_WARN(
            "failed to find android system property \"ro.build.version.sdk\"");
        return;
    }

    if (sdk_ver >= 28) {
        SENTRY_DEBUG("running on android >= 28; load OS unwinder");
        handle = dlopen("libandroid_unwinder_os.so", RTLD_LOCAL | RTLD_NOW);
    } else {
        SENTRY_DEBUG("running on android < 28; load legacy unwinder");
        handle = dlopen("libandroid_unwinder_legacy.so", RTLD_LOCAL | RTLD_NOW);
    }
    if (!handle) {
        SENTRY_WARN("failed to load the legacy unwinder library");
        return;
    }
    android_unwind_stack
        = (android_unwind_stack_t)dlsym(handle, "android_unwind_stack");
    if (!android_unwind_stack) {
        SENTRY_WARN("failed to load unwinder function symbol");
    }
}

void
sentry__unload_unwinder()
{
    int result = dlclose(handle);
    if (result) {
        SENTRY_WARN(dlerror());
    }
    android_unwind_stack = NULL;
}
