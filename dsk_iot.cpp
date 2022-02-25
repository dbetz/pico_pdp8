#include <stdint.h>

#include "nano8.h"

static unsigned int dskrg, dskmem, dskfl, tm, i;
static unsigned int dskad;
static unsigned int tmp;
static uint8_t  *p;

void dsk_iot()
{
    switch (inst & 0777) {
    case 0601:
        dskad = dskfl = 0;
        break;
    case 0605:
    case 0603:
        i = (dskrg & 070) << 9;
        dskmem = ((mem[07751] + 1) & 07777) | i; /* mem */
        tm = (dskrg & 03700) << 6;
        dskad = (acc & 07777) + tm; /* dsk */
        i = 010000 - mem[07750];
        p = (uint8_t *)&mem[dskmem];
        dsk_seek(dskad * 2);
        if (inst & 2) {
            /*read */
            //digitalWrite(R_LED, LOW);
            tmp = dsk_read(p, i * 2);
            //digitalWrite(R_LED, HIGH);
            //Serial.printf("Read:%o>%o:%o,%d Len:%d\r\n", dskad, dskmem, dskrg, i, tmp);
        }
        else {
            //digitalWrite(R_LED, LOW);
            tmp = dsk_write(p, i * 2);
            //digitalWrite(R_LED, HIGH);
            //Serial.printf("Write:%o>%o:%o,%d Len:%d\r\n", dskad, dskmem, dskrg, i, tmp);
        }
        dskfl = 1;
        mem[07751] = 0;
        mem[07750] = 0;
        acc = acc & 010000;
        break;
    case 0611:
        dskrg = 0;
        break;
    case 0615:
        dskrg = (acc & 07777); /* register */
        break;
    case 0616:
        acc = (acc & 010000) | dskrg;
        break;
    case 0626:
        acc = (acc & 010000) + (dskad & 07777);
        break;
    case 0622:
        if (dskfl) pc++;
        break;
    case 0612:
        acc = acc & 010000;
    case 0621:
        pc++; /* No error */
        break;
    }
}

void dsk_clear()
{
    dskfl = 0;
}
