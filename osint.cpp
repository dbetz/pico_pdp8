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

ms_time_t mstime()
{
    return (ms_time_t)to_ms_since_boot(get_absolute_time());
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
// 26 maps to PiDP GPIO header pin 19 (the   doesn't provide 26)
static uint8_t cols[NCOLS] = {13, 12, 11,    10, 9, 8,    7, 6, 5,    4, 15, 14};
static uint8_t ledrows[NLEDROWS] = {20, 21, 22, 2, 3, 28, 26, 27};
static uint8_t rows[NROWS] = {16, 17, 18};

void test_panel_led()
{
    // Enable GPIO function on all pins
    for (size_t i = 0; i < NCOLS; i++) {
        gpio_init(cols[i]);
        gpio_set_dir(cols[i], GPIO_OUT);
    }
    for (size_t i = 0; i < NLEDROWS; i++) {
        gpio_init(ledrows[i]);
        gpio_set_dir(ledrows[i], GPIO_OUT);
    }
    for (size_t i = 0; i < NLEDROWS; i++) {
        gpio_put(ledrows[i], 1);
        for (size_t j = 0; j < NCOLS; j++) {
            gpio_put(cols[j], 0);
        }
    }
}

//// turn_on/off_pidp8i_leds ///////////////////////////////////////////
// Set GPIO pins into a state that [dis]connects power to/from the LEDs.
// Doesn't pay any attention to the panel values.

void turn_on_pidp8i_leds ()
{
    for (size_t row = 0; row < NLEDROWS; ++row) {
        gpio_set_dir (ledrows[row], GPIO_IN);
        gpio_clr_mask(1 << ledrows[row]);
    }
    for (size_t col = 0; col < NCOLS; ++col) {
        gpio_set_dir (cols[col], GPIO_IN);
    }
}

void turn_off_pidp8i_leds ()
{
    for (size_t col = 0; col < NCOLS; ++col) {
        gpio_set_dir (cols[col], GPIO_OUT);
    }
    for (size_t row = 0; row < NLEDROWS; ++row) {
        gpio_set_dir (ledrows[row], GPIO_IN);
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
        gpio_set_dir (rows[i], GPIO_IN);
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
    //uint16_t *pcurr = pdis_paint->curr;
    uint16_t *pcurr = pdis_update->curr;

    // Override Execute and Run LEDs if the CPU is currently stopped,
    // since we only get set_pidp8i_leds calls while the CPU's running.
    if (swStop || swSingInst) {
        pdis_paint->curr[5] &= ~(1 << 2);
        pdis_paint->curr[6] &= ~(1 << 7);
    }

    for (size_t row = 0; row < NLEDROWS; ++row) {
        uint32_t setMask = 0;
        uint32_t clrMask = 0;
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
        gpio_set_dir (ledrows[row], GPIO_IN);
        gpio_set_mask(1 << ledrows[row]);
        gpio_set_dir (ledrows[row], GPIO_OUT);

        sleep_us (delay);

        // Toggle this LED row off
        gpio_clr_mask(1 << ledrows[row]); // superstition
        gpio_set_dir (ledrows[row], GPIO_IN);

        // Small delay to reduce UDN2981 ghosting
        sleep_us (10);
    }
}

int gss_initted = 0;

//// read_switches /////////////////////////////////////////////////////
// Iterate through the switch GPIO pins, passing them to the debouncing
// mechanism above for eventual reporting to the PDP-8 CPU thread.

void read_switches (uint64_t delay)
{
    // Save current ms-since-epoch for debouncer.  No point making it
    // retrieve this value for each switch.
    ms_time_t now_ms = mstime();

    // Flip columns to input.  Since the internal pull-ups are enabled,
    // this pulls all switch GPIO pins high that aren't shorted to the
    // row line by the switch.
    for (size_t i = 0; i < NCOLS; ++i) {
        gpio_set_dir(cols[i], GPIO_IN);
    }

    // Read the switch rows
    for (size_t i = 0; i < NROWS; ++i) {
        // Put 0V out on the switch row so that closed switches will
        // drag its column line down; give it time to settle.
        gpio_set_dir(rows[i], GPIO_OUT);
        gpio_clr_mask(1 << rows[i]);
        sleep_us (delay);

        // Read all the switches in this row
        for (size_t j = 0; j < NCOLS; ++j) {
            int ss = gpio_get(cols[j]);
            debounce_switch(i, j, !!ss, now_ms);
        }

        // Stop sinking current from this row of switches
        gpio_set_dir(rows[i], GPIO_IN);
    }

    gss_initted = 1;
}



