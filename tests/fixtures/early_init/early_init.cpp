#include <sentry.h>

static int
do_early_init()
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, true);
    return sentry_init(options);
}

#if defined(_MSC_VER)
// call sentry_init() as early as possible, before other static initializers
#    pragma init_seg(lib)
#endif
static int early_init = do_early_init();

int
main()
{
    sentry_close();
    return early_init;
}
