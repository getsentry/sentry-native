#ifndef SENTRY_APP_HANG_SAMPLER_H_INCLUDED
#define SENTRY_APP_HANG_SAMPLER_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

// A platform thread sampler is compiled in only for the targets below (see the
// `app-hang platform sampler` block in src/CMakeLists.txt). On any other target
// `sentry__app_hang_sample_thread` has no definition, so the monitor must not
// reference it there.
#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)                \
    || defined(__ANDROID__)
#    define SENTRY_HAS_APP_HANG_SAMPLER 1
#else
#    define SENTRY_HAS_APP_HANG_SAMPLER 0
#endif

#if SENTRY_HAS_APP_HANG_SAMPLER
size_t sentry__app_hang_sample_thread(
    uint64_t target_tid, void **ips_out, size_t max_frames);
#endif

#endif
