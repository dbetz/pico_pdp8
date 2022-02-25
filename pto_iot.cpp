#include "nano8.h"

static short pto;

void pto_iot()
{
    switch (inst & 07)
    {
    case 1:
        if (pto >= TTWAIT)
            pc++;
        break;
    case 2:
        pto = 0;
        break;
    case 6:
    case 4:
        pto = acc & 0177;
#if 0
        if (ptpfile == 0)
            f_write(&ptwrite, &pto, 1, &bcnt);
#endif
        pto = 1;
        break;
    }
}

void pto_clear()
{
    pto = 0;
}

void pto_idle()
{
	if (pto && (pto < TTWAIT))
		pto++;
}
