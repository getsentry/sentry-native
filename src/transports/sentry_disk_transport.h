#ifndef SENTRY_TRANSPORTS_DISK_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORTS_DISK_TRANSPORT_H_INCLUDED

#include "../sentry_boot.h"
#include "../sentry_path.h"

sentry_transport_t *sentry_new_disk_transport(
    const sentry_path_t *database_path);

#endif