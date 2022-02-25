#include "nano8.h"

static short tti;
static short ttf;

void tti_iot()
{
    switch (inst & 07)
    {
    case 1:
        if (ttf)
            pc++;
        break;
    case 2:
        ttf = 0;
        acc &= 010000;
        break;
    case 4:
        acc |= tti | 0200;
        break;
    case 6:
        acc &= 010000;
        acc |= tti | 0200;
        ttf = 0;
    }
}

void tti_clear()
{
    tti = ttf = 0;
}

bool tti_idle()
{
    if (!ttf) {
        int ch = serial_getchar();
        if (ch != -1) {
            tti = ch;
            ttf = 1;
        }
    }
    if (tti == 1 || tti == 5)						// Halt on keypress ^a or ^e
        return false;
    return true;
}

bool tti_interrupt()
{
    return ttf != 0;
}
