#include <string.h>
#include "sentrypad.h"

int main(void)
{
    sentrypad_init();

    sentrypad_set_extra("extra stuff", "some value");
    sentrypad_set_tag("some tag", "some value");
    sentrypad_set_release("5fd7a6cd");

    memset((char *)0x0, 1, 100);
}