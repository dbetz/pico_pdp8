#include <stdint.h>

#include "nano8.h"

static short rkdn, rkcmd, rkca, rkwc;
static int rkda;
static uint8_t  *p;
#if 0
static File rk05;
#endif

void rk8e_iot()
{
    //      printf("Acc:%04o IOT:%04o\n",acc,xmd);
    switch (inst & 7)
    {
    case 0:
        break;
    case 1:
        if (rkdn) {
            pc++;
            rkdn = 0;
        }
        break;
    case 2:
        acc &= 010000;
        rkdn = 0;
        break;
    case 3:
        rkda = acc & 07777;
        //
        // OS/8 Scheme. 2 virtual drives per physical drive
        // Regions start at 0 and 6260 (octal).
        //
        acc &= 010000;
        if (rkcmd & 6) {
            //printf("Unit error\n");
            return;
        }
        switch (rkcmd & 07000)
        {
        case 0:
        case 01000:
            rkca |= (rkcmd & 070) << 9;
            rkwc = (rkcmd & 0100) ? 128 : 256;
            rkda |= (rkcmd & 1) ? 4096 : 0;
            //f_lseek(&rk05, rkda * 512);
#if 0
            rk05.seek(rkda * 512, SeekSet);
#endif
            p = (uint8_t *)&mem[rkca];
            //f_read(&rk05, p, rkwc * 2, &bcnt);
#if 0
            tmp = rk05.read(p, rkwc * 2);
#endif
            //Serial.printf("Read Mem:%04o Dsk:%04o Len:%d\r\n", rkca, rkda, tmp);
            rkca = (rkca + rkwc) & 07777;
            rkdn++;
            // printf(".");
            break;
        case 04000:
        case 05000:
            rkca |= (rkcmd & 070) << 9;
            rkwc = (rkcmd & 0100) ? 128 : 256;
            rkda |= (rkcmd & 1) ? 4096 : 0;
            //printf("Write Mem:%04o Dsk:%04o\n",rkca,rkda);
            //fseek(rk05,rkda*512,SEEK_SET);
            //f_lseek(&rk05, rkda * 512);
#if 0
            rk05.seek(rkda * 512, SeekSet);
#endif
            p = (uint8_t *)&mem[rkca];
            //fwrite(p,2,rkwc,rk05);
            //f_write(&rk05, p, rkwc * 2, &bcnt);
#if 0
            tmp = rk05.write(p, rkwc * 2);
#endif
            //rk05.flush();
            //Serial.printf("Write Mem:%04o Dsk:%04o Len:%d\r\n", rkca, rkda, tmp);
            rkca = (rkca + rkwc) & 07777;
            rkdn++;
            break;
        case 02000:
            break;
        case 03000:
            if (rkcmd & 0200) rkdn++;
            break;
        }
        break;
    case 4:
        rkca = acc & 07777;
        acc &= 010000;
        break;
    case 5:
        //        acc=(acc&010000)|(rkdn?04000:0);
        acc = (acc & 010000) | 04000;
        break;
    case 6:
        rkcmd = acc & 07777;
        acc &= 010000;
        break;
    case 7:
        //printf("Not allowed...RK8E\n");
        break;
    }
}
