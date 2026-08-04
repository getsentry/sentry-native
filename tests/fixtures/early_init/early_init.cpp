#include <sentry.h>

static int
do_early_init()
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, true);
    return sentry_init(options);
}

struct EarlyInit {
    int result = do_early_init();
};

#if defined(_MSC_VER)
// call sentry_init() as early as possible, before other static initializers
#    pragma init_seg(lib)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((init_priority(101)))
#endif
static EarlyInit early_init;

int
main()
{
    sentry_close();
    return early_init.result;
}
