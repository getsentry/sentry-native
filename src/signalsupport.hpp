#ifndef SENTRY_SIGNALSUPPORT_HPP_INCLUDED
#define SENTRY_SIGNALSUPPORT_HPP_INCLUDED

#include <atomic>

namespace sentry {
extern std::atomic_bool g_is_in_terminating_signal_handler;

/* if called we're going to tear down anyways and for safety
   are going to disable some concurrency systems */
void enter_terminating_signal_handler();

/* are we in a terminating signal handler */
bool is_in_terminating_signal_handler() {
    return g_is_in_terminating_signal_handler.load();
}
}  // namespace sentry

#endif
