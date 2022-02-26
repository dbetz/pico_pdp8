/*
 * Based on PiDP-8i source code
 *
 * Copyright © 2015 Oscar Vermeulen, © 2016-2018 by Warren Young
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS LISTED ABOVE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors above shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from those
 * authors.
 *
 * www.obsolescenceguaranteed.blogspot.com
 * 
 * This is part of the GPIO thread, which communicates with the
 * simulator's main thread via *pdis_update and switchstatus[].
 * All of this module's other external interfaces are only called
 * by the other gpio-* modules, from the GPIO thread.
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "panel_leds.h"

// from the SIMH PDP8 source code
#define INT_V_START     0                               /* enable start */
#define INT_V_DIRECT    (INT_V_START+14)                /* direct start */
#define INT_V_OVHD      (INT_V_DIRECT+12)               /* overhead start */
#define INT_V_ION       (INT_V_OVHD+2)                  /* interrupts on */
#define INT_ION         (1 << INT_V_ION)

// Double-buffered LED display brightness values.  The update-to copy is
// modified by the SIMH CPU thread as it executes instructions, and the
// paint-from copy is read by the gpio-*.c module in its "set LEDs"
// loop.  When the GPIO thread is finished with the paint-from copy, it
// zeroes it and swaps it for the current "update-to" copy, giving the
// CPU thread a blank slate, and giving the GPIO thread a stable set of
// LED "on" time values to work with.
display display_bufs[2];
display* pdis_update = display_bufs + 0;    // exported to SIMH CPU thread
display* pdis_paint  = display_bufs + 1;    // exported to gpio-*.c

//// set_pidp8i_led ////////////////////////////////////////////////////
// Sets the current state for a single LED at the given row and column
// on the PiDP-8/I PCB.  Also increments the LED on-count value for 
// that LED.
//
// You may say, "You can't just use the C postincrement operator here!
// Look at the assembly output!  You must use an atomic increment for
// this!"  And indeed, there is a big difference between the two
// methods: https://godbolt.org/g/0Qt0Ap
//
// The thing is, both structures referred to by pdis_* are fixed in RAM,
// and the two threads involved are arranged in a strict producer-and-
// consumer fashion, so it doesn't actually matter if pdis_update gets
// swapped for pdis_paint while we're halfway through an increment: we
// get a copy of the pointer to dereference here, so we'll finish our
// increment within the same structure we started with, even if
// pdis_update points at the other display structure before we leave.

static inline void set_pidp8i_led (display *pd, size_t row, size_t col)
{
    ++pd->on[row][col];
    pd->curr[row] |= 1 << col;
}


//// set_pidp8i_row_leds ///////////////////////////////////////////////
// Like set_pidp8i_led, except that it takes a 12-bit state value for
// setting all LEDs on the given row.  Because we copy the pdis_update
// pointer before making changes, if the display swap happens while
// we're working, we'll simply finish updating what has become the
// paint-from display, which is what you want; you don't want the
// updates spread over both displays.

static inline void set_pidp8i_row_leds (display *pd, size_t row,
        uint16_t state)
{
    size_t *prow = pd->on[row];
    pd->curr[row] = state;
    for (size_t col = 0, mask = 1; col < NCOLS; ++col, mask <<= 1) {
        if (state & mask) ++prow[col];
    }
}


//// set_3_pidp8i_leds /////////////////////////////////////////////////
// Special case of set_pidp8i_row_leds for the DF and IF LEDs: we only
// pay attention to bits 12, 13, and 14 of the given state value,
// because SIMH's PDP-8 simulator shifts those 3 bits up there so it can
// simply OR these 3-bit registers with PC to produce a 15-bit extended
// address.
//
// We don't take a row parameter because we know which row they're on,
// but we do take a column parameter so we can generalize for IF & DF.

static inline void set_3_pidp8i_leds (display *pd, size_t col,
        uint16_t state)
{
    static const int row = 7;       // DF and IF are on row 6
    size_t *prow = pd->on[row];
    size_t last_col = col + 3;
    pd->curr[row] |= state >> (12 - col);
    for (size_t mask = 1 << 12; col < last_col; ++col, mask <<= 1) {
        if (state & mask) ++prow[col];
    }
}


//// set_5_pidp8i_leds /////////////////////////////////////////////////
// Like set_3... but for the 5-bit SC register.  Because it's only used
// for that purpose, we don't need the col parameter.

static inline void set_5_pidp8i_leds (display *pd, uint16_t state)
{
    static const int row = 6;       // SC is on row 6
    size_t *prow = pd->on[row];
    size_t last_col = 7;
    pd->curr[row] |= (state & 0x1f) << 2;
    for (size_t col = 2, mask = 1; col < last_col; ++col, mask <<= 1) {
        if (state & mask) ++prow[col];
    }
}

//// set_pidp8i_leds ///////////////////////////////////////////////////
// Given all of the PDP-8's internal registers that affect the front
// panel display, modify the GPIO thread's LED state values accordingly.
//
// Also update the LED brightness values based on those new states.

void set_pidp8i_leds (uint32_t sPC, uint32_t sMA, uint32_t sMB,
    uint16_t sIR, int32_t sLAC, int32_t sMQ, int32_t sIF, int32_t sDF,
    int32_t sSC, int32_t int_req, int Pause)
{
    // Bump the instruction count.  This should always be equal to the
    // Fetch LED's value, but integers are too cheap to get cute here.
    //
    // Note that we only update pdis_update directly once in this whole
    // process.  This is in case the display swap happens while we're
    // working: we want to finish work on the same display even though
    // it's now called the paint-from display, so it's consistent.
    display* pd = pdis_update;
    ++pd->inst_count;

    // Rows 0-4, easy cases: single-register LED strings.
    // 
    // The values passed for rows 1 and 2 are non-obvious.  See the code
    // calling us from ../SIMH/PDP8/pdp8_cpu.c for details.
    set_pidp8i_row_leds (pd, 0, sPC);
    set_pidp8i_row_leds (pd, 1, sMA);
    set_pidp8i_row_leds (pd, 2, sMB);
    set_pidp8i_row_leds (pd, 3, sLAC & 07777);
    set_pidp8i_row_leds (pd, 4, sMQ);

#if 0   // debugging
    static time_t last = 0, now;
    if (time(&now) != last) {
        uint16_t* pcurr = pd->curr;
        printf("\r\nSET: [PC:%04o] [MA:%04o] [MB:%04o] [AC:%04o] [MQ:%04o]",
                pcurr[0], pcurr[1], pcurr[2], pcurr[3], pcurr[4]);
        last = now;
    }
#endif

    // Row 5a: instruction type column, decoded from high octal
    // digit of IR value
    pd->curr[5] = 0;
    uint16_t inst_type = sIR & 07000;
    switch (inst_type) {
        case 00000: set_pidp8i_led (pd, 5, 11); break; // 000 AND
        case 01000: set_pidp8i_led (pd, 5, 10); break; // 001 TAD
        case 02000: set_pidp8i_led (pd, 5,  9); break; // 010 DCA
        case 03000: set_pidp8i_led (pd, 5,  8); break; // 011 ISZ
        case 04000: set_pidp8i_led (pd, 5,  7); break; // 100 JMS
        case 05000: set_pidp8i_led (pd, 5,  6); break; // 101 JMP
        case 06000: set_pidp8i_led (pd, 5,  5); break; // 110 IOT
        case 07000: set_pidp8i_led (pd, 5,  4); break; // 111 OPR 1 & 2
    }

    // Row 5b: set the Defer LED if...
    if ((inst_type <= 05000) &&  // it's a memory reference instruction
            (sIR & 00400)) {     // and indirect addressing flag is set
        set_pidp8i_led (pd, 5, 1);
    }

    // Row 5c: The Fetch & Execute LEDs are pulsed once per instruction.
    // On real hardware, the pulses don't happen at exactly the same
    // time, but we can't simulate that because SIMH handles each CPU
    // instruction "whole."  When running real code, all we care about
    // is that both LEDs are twiddled so rapidly that they both just
    // become a 50% blur, mimicking the hardware closely enough.
    //
    // The exception is that when the CPU is stopped, both LEDs are off,
    // because the pulses happen "outside" the STOP state: Fetch before
    // and Execute after resuming from STOP.
    extern int swStop, swSingInst;
    int running = !swStop && !swSingInst;
    if (running) {
        set_pidp8i_led (pd, 5, 2);    // Execute
        set_pidp8i_led (pd, 5, 3);    // Fetch
    }

    // Row 6a: Remaining LEDs in upper right block
    pd->curr[6] = 0;
    if (running)           set_pidp8i_led (pd, 6, 7); // bump Run LED
    if (Pause)             set_pidp8i_led (pd, 6, 8); // bump Pause LED
    if (int_req & INT_ION) set_pidp8i_led (pd, 6, 9); // bump ION LED

    // Row 6b: The Step Count LEDs are also on row 6
    set_5_pidp8i_leds (pd, sSC);

    // Row 7: DF, IF, and Link.
    pd->curr[7] = 0;
    set_3_pidp8i_leds (pd, 9, sDF);
    set_3_pidp8i_leds (pd, 6, sIF);
    if (sLAC & 010000) set_pidp8i_led (pd, 7, 5);

    // If we're stopped or single-stepped, the display-swapping code
    // won't happen, so copy the above over to the paint-from version.
    extern int resumeFromInstructionLoopExit;
    if (!running || resumeFromInstructionLoopExit) {
        memcpy(pdis_paint, pdis_update, sizeof(struct display));
    }
}
