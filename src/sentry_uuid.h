
#ifndef SENTRY_UUID_H_INCLUDED
#define SENTRY_UUID_H_INCLUDED

#include "sentry_boot.h"

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_uuid_t sentry__uuid_from_native(GUID *guid);
#endif

#endif