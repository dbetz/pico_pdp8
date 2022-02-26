#include <stdio.h>
#include "pico/stdlib.h"

#include "nano8.h"
#include "panel_leds.h"
#include "panel_switches.h"

void serial_putchar(int ch)
{
	putchar_raw(ch);
}

int serial_getchar()
{
	int ch = getchar_timeout_us(0);
	return ch == PICO_ERROR_TIMEOUT ? -1 : ch;
}

void dsk_seek(unsigned int addr)
{
}

size_t dsk_read(void *buffer, size_t size)
{
    return -1;
}

size_t dsk_write(void *buffer, size_t size)
{
    return -1;
}

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

// 2  maps to PiDP GPIO header pin 23
// 3  maps to PiDP GPIO header pin 24
// 28 maps to PiDP GPIO header pin 25
static uint8_t cols[NCOLS] = {13, 12, 11,    10, 9, 8,    7, 6, 5,    4, 15, 14};
static uint8_t ledrows[NLEDROWS] = {20, 21, 22, 2, 3, 28, 26, 27};
static uint8_t rows[NROWS] = {16, 17, 18};

//// turn_on/off_pidp8i_leds ///////////////////////////////////////////
// Set GPIO pins into a state that [dis]connects power to/from the LEDs.
// Doesn't pay any attention to the panel values.

void turn_on_pidp8i_leds ()
{
    for (size_t row = 0; row < NLEDROWS; ++row) {
        gpio_set_dir (ledrows[row], false);
        gpio_clr_mask(1 << ledrows[row]);
    }
    for (size_t col = 0; col < NCOLS; ++col) {
        gpio_set_dir (cols[col], false);
    }
}

void turn_off_pidp8i_leds ()
{
    for (size_t col = 0; col < NCOLS; ++col) {
        gpio_set_dir (cols[col], true);
    }
    for (size_t row = 0; row < NLEDROWS; ++row) {
        gpio_set_dir (ledrows[row], false);
    }
}

//// init_pidp8i_gpio //////////////////////////////////////////////////
// Initialize the GPIO pins to the initial states required by
// gpio_thread().   It's a separate exported function so that scanswitch
// can also use it.

void init_pidp8i_gpio (void)
{
    // Enable GPIO function on all pins
    for (size_t i = 0; i < NCOLS; i++) {
        gpio_init(cols[i]);
    }
    for (size_t i = 0; i < NLEDROWS; i++) {
        gpio_init(ledrows[i]);
    }
    for (size_t i = 0; i < NROWS; i++) {
        gpio_init(rows[i]);
    }

    // Set GPIO pins to their starting state
    turn_on_pidp8i_leds ();
    for (size_t i = 0; i < NROWS; i++) {       // Define rows as input
        gpio_set_dir (rows[i], false);
    }
    
    // Set pull-up's
    for (size_t i = 0; i < NCOLS; i++) {
        gpio_pull_up(cols[i]);
    }
    
    // Set pull-down's
    for (size_t i = 0; i < NLEDROWS; i++) {
        gpio_pull_down(ledrows[i]);
    }
    
    // Set float's
    for (size_t i = 0; i < NROWS; i++) {
        gpio_disable_pulls(rows[i]);
    }
}


//// update_led_states /////////////////////////////////////////////////
// Generic front panel LED updater used by NLS full time and by ILS
// while the CPU is in STOP mode.  Just uses the paint-from display's
// bitfields to turn the LEDs on full-brightness.

void update_led_states (uint64_t delay)
{
    uint16_t *pcurr = pdis_paint->curr;

    // Override Execute and Run LEDs if the CPU is currently stopped,
    // since we only get set_pidp8i_leds calls while the CPU's running.
    if (swStop || swSingInst) {
        pdis_paint->curr[5] &= ~(1 << 2);
        pdis_paint->curr[6] &= ~(1 << 7);
    }

    uint32_t setMask = 0;
    uint32_t clrMask = 0;
    for (size_t row = 0; row < NLEDROWS; ++row) {
        for (size_t col = 0; col < NCOLS; ++col) {
            if ((pcurr[row] & (1 << col)) == 0) {
                setMask |= 1 << cols[col];
            }
            else {
                clrMask |= 1 << cols[col];
            }
        }
        gpio_set_mask(setMask);
        gpio_clr_mask(clrMask);

        // Toggle this LED row on
        gpio_set_dir (ledrows[row], false);
        gpio_set_mask(1 << ledrows[row]);
        gpio_set_dir (ledrows[row], true);

        sleep_us (delay);

        // Toggle this LED row off
        gpio_clr_mask(1 << ledrows[row]); // superstition
        gpio_set_dir (ledrows[row], false);

        // Small delay to reduce UDN2981 ghosting
        sleep_us (10);
    }
}


