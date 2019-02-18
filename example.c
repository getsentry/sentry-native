#include <string.h>
#include "sentrypad.h"

int main(void)
{
    sentrypad_init();

    memset((char *)0x0, 1, 100);
}