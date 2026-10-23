#ifndef SENTRY_SCREENSHOT_H_INCLUDED
#define SENTRY_SCREENSHOT_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_options.h"
#include "sentry_path.h"

/**
 * Captures a screenshot and saves it to the specified path.
 *
 * @param path The path where the screenshot should be saved.
 * @param pid The process ID whose windows should be captured (0 = current
 * process).
 *
 * Returns 0 if the screenshot was successfully captured and saved.
 */
int sentry_screenshot_capture(const char *path, uint32_t pid);

/**
 * Returns the path where a screenshot should be saved.
 */
sentry_path_t *sentry__screenshot_get_path(const sentry_options_t *options);

#endif
