#include "sentry_random.h"

#if SENTRY_PLATFORM == SENTRY_PLATFORM_MACOS
#    include <stdlib.h>

static int
getrandom_arc4random(void *dst, size_t bytes)
{
    arc4random_buf(dst, bytes);
    return 0;
}
#    define HAVE_ARC4RANDOM

#endif
#if SENTRY_PLATFORM != SENTRY_PLATFORM_WINDOWS
#    include <errno.h>
#    include <fcntl.h>
#    include <unistd.h>

static int
getrandom_devurandom(void *dst, size_t bytes)
{
    char *d = dst;
    size_t to_read = bytes;
    int fd, res;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return 1;
    }

    while (to_read > 0) {
        res = read(fd, d, to_read);
        if (res > 0) {
            to_read -= res;
            d += res;
        } else if (errno == EINTR) {
            continue;
        } else {
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}

#    define HAVE_URANDOM
#endif

int
sentry__getrandom(void *dst, size_t len)
{
#ifdef HAVE_ARC4RANDOM
    if (getrandom_arc4random(dst, len) == 0) {
        return 0;
    }
#endif
#ifdef HAVE_URANDOM
    if (getrandom_devurandom(dst, len) == 0) {
        return 0;
    }
#endif
    return 1;
}