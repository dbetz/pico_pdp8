#pragma once

#include <stdlib.h>
#include <stdint.h>

// Number of LED rows and columns on the PiDP-8/I PCB
#define NCOLS    12
#define NLEDROWS 8
 
typedef struct display {
    // Counters incremented each time the LED is known to be turned on,
    // in instructions since the last display paint.
    size_t on[NLEDROWS][NCOLS];

    // Most recent state for each LED, for use by NLS full-time and by
    // ILS in STOP mode.  (One bitfield per row.)
    uint16_t curr[NLEDROWS];

    // Number of instructions executed since this display was cleared
    int inst_count;
} display;

extern display display_bufs[2];
extern display* pdis_update;   // exported to SIMH CPU thread
extern display* pdis_paint;    // exported to gpio-*.c

void set_pidp8i_leds (uint32_t sPC, uint32_t sMA, uint32_t sMB,
    uint16_t sIR, int32_t sLAC, int32_t sMQ, int32_t sIF, int32_t sDF,
    int32_t sSC, int32_t int_req, int Pause);

