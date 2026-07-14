#include "sentry_session_replay.h"

#include "sentry_core.h"

bool
sentry__session_replay_capture(const sentry_path_t *UNUSED(path),
    uint32_t UNUSED(duration_ms), uint32_t UNUSED(pid),
    uint64_t UNUSED(end_ts_ms), sentry_session_replay_info_t *UNUSED(info))
{
    return false;
}
