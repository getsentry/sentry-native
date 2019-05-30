#include "cleanup.hpp"
#include "options.hpp"
#include "path.hpp"

void sentry::cleanup_old_runs() {
    const sentry_options_t *options = sentry_get_options();
    sentry::Path run_path = options->database_path.join(SENTRY_RUNS_FOLDER);

    // for now we delete all old runs but in the ideal case we would leave
    // runs alone of invocations of the application that is still alive.
    bool some_failed = false;
    sentry::PathIterator iter = run_path.iter_directory();
    while (iter.next()) {
        if (iter.path()->filename_matches(options->run_id.c_str())) {
            continue;
        }
        if (!iter.path()->remove_all()) {
            some_failed = true;
        }
    }

    if (some_failed) {
        SENTRY_PRINT_ERROR("failed to remove old sentry run");
    }
}
