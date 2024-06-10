#include <sentry.h>
#include <stdio.h>

long my_filter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    printf("I should never execute\n");
    printf("If you see this we failed\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

int
main()
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://d545f4da00ff7b2fa7b6c8620c94a4e9@o447951.ingest.sentry.io/4506178389999616");
    sentry_options_set_debug(options, 1);
    sentry_options_set_uef_lock(options, 1);
    sentry_init(options);

    SetUnhandledExceptionFilter(my_filter);

    Sleep(5000);

    int* ptr = (int *)100;
    memset(ptr, 100, 100);

    sentry_close();
}
