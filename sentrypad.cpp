#include "crashpad_wrapper.h"
#include "sentrypad.h"

int sentrypad_init()
{
    return initialize_crashpad();
}