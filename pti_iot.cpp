#include "nano8.h"

static short pti;

void pti_iot()
{
    switch (inst & 07)
    {
    case 01:
        if (pti > -1)
            pc++;
        break;
    case 02:
        acc |= pti & 0377;
        pti = -2;
        break;
    case 04:
        pti = -1;
        break;
    case 06:
        acc |= pti & 0377;
        pti = -1;
        break;
    }
}

void pti_clear()
{
	pti = -2; // Flag clear until explicit fetch
}

void pti_idle()
{
#if 0
    if (ptrfile == 0) {
        if (pti == -1 && !f_eof(&ptread)) {
            pti = 0;
            f_read(&ptread, &pti, 1, &bcnt);
        }
    }
#endif
}
