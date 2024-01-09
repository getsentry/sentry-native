#include "sentry_test_util.h"

#include <string.h>

#ifdef SENTRY_PLATFORM_AIX
// AIX has a null page mapped to the bottom of memory, which means null derefs
// don't segfault. try dereferencing the top of memory instead; the top nibble
// seems to be unusable.
static void *invalid_mem = (void *)0xFFFFFFFFFFFFFF9B; // -100 for memset
#else
static void *invalid_mem = (void *)1;
#endif

void
sentry_trigger_crash(void)
{
    memset((char *)invalid_mem, 1, 100);
}