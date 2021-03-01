#ifndef SENTRY_PROCMAPS_MODULEFINDER_ELF_H_INCLUDED
#define SENTRY_PROCMAPS_MODULEFINDER_ELF_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_slice.h"

bool sentry__procmaps_read_ids_from_elf(sentry_value_t value, void *elf_ptr);

#endif
