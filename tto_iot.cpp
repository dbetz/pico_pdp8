#include "nano8.h"

static short tto;

void tto_iot()
{
    switch (inst & 07)
    {
    case 1:
        if (tto >= TTWAIT)
            pc++;
        break;
    case 2:
        tto = 0;
        break;
    case 6:
    case 4:
        serial_putchar(acc & 0177);
        tto = 1;
        break;
    }
}

void tto_clear()
{
    tto = 0;
}

void tto_idle()
{
	if (tto && (tto < TTWAIT)) {
	    if ((tto % 100) == 0) printf("tto %d\n", tto);
		tto++;
	}
}

bool tto_interrupt()
{
    return tto == TTWAIT;
}
