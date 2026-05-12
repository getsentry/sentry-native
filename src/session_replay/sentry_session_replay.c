#include "sentry_session_replay.h"

sentry_path_t *
sentry__session_replay_get_path(const sentry_options_t *options)
{
    return sentry__path_join_str(options->run->run_path, "session-replay.mp4");
}
