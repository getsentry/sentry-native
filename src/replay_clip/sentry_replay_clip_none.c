#include "sentry_replay_clip.h"

#include "sentry_core.h"

bool
sentry__replay_clip_capture(const sentry_path_t *UNUSED(path),
    uint32_t UNUSED(duration_ms), uint32_t UNUSED(pid))
{
    return false;
}
