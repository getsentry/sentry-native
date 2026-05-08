#include "sentry_replay_clip.h"

sentry_path_t *
sentry__replay_clip_get_path(const sentry_options_t *options)
{
    return sentry__path_join_str(options->run->run_path, "replay-clip.mp4");
}
