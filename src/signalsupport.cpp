#include "signalsupport.hpp"

std::atomic_bool sentry::g_is_in_terminating_signal_handler;

void sentry::enter_terminating_signal_handler() {
    sentry::g_is_in_terminating_signal_handler.store(true);
}