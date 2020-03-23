#ifndef SENTRY_MODULEFINDER_H_INCLUDED
#define SENTRY_MODULEFINDER_H_INCLUDED

#include "sentry_boot.h"

sentry_value_t sentry__modules_get_list(void);

void sentry__modulefinder_cleanup(void);

#endif
