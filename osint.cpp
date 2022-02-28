#include <stdio.h>
#include "pico/stdlib.h"

#include "nano8.h"
#include "panel_leds.h"
#include "panel_switches.h"

extern "C" {
    #include "fs9p.h"
}

void serial_putchar(int ch)
{
	putchar_raw(ch);
}

int serial_getchar()
{
	int ch = getchar_timeout_us(0);
	return ch == PICO_ERROR_TIMEOUT ? -1 : ch;
}

fs9_file f;
bool finit = false;

// receive 1 byte
static unsigned int zdoGet1()
{
    int c;
    do {
        c = serial_getchar();
    } while (c < 0);
    return c;
}

// receive an unsigned long
static unsigned int zdoGet4()
{
    unsigned r;
    r = zdoGet1();
    r = r | (zdoGet1() << 8);
    r = r | (zdoGet1() << 16);
    r = r | (zdoGet1() << 24);
    return r;
}

// send a buffer to the host
// then receive a reply
// returns the length of the reply, which is also the first
// longword in the buffer
//
// startbuf is that start of the buffer (used for both send and
// receive); endbuf is the end of data to send; maxlen is maximum
// size
static int plain_sendrecv(uint8_t *startbuf, uint8_t *endbuf, int maxlen)
{
    int len = endbuf - startbuf;
    uint8_t *buf = startbuf;
    int i = 0;
    int left;
    unsigned flags;

    startbuf[0] = len & 0xff;
    startbuf[1] = (len>>8) & 0xff;
    startbuf[2] = (len>>16) & 0xff;
    startbuf[3] = (len>>24) & 0xff;

    if (len <= 4) {
        return -1; // not a valid message
    }
    // loadp2's server looks for magic start sequence of $FF, $01
    serial_putchar(0xff);
    serial_putchar(0x01);
    while (len>0) {
        serial_putchar(*buf++);
        --len;
    }
    len = zdoGet4();
    startbuf[0] = len & 0xff;
    startbuf[1] = (len>>8) & 0xff;
    startbuf[2] = (len>>16) & 0xff;
    startbuf[3] = (len>>24) & 0xff;
    buf = startbuf+4;
    left = len - 4;
    while (left > 0 && i < maxlen) {
        buf[i++] = zdoGet1();
        --left;
    }
    return len;
}

int dsk_init(const char *path)
{
    if (fs_init(plain_sendrecv) == 0) {
        if (fs_open(&f, path, FS_MODE_RDWR) == 0) {
            finit = true;
            return 0;
        }
    }
    return -1;
}

int dsk_seek(unsigned int addr)
{
    //printf("seek %u\n", addr);
    return finit ? fs_seek(&f, (uint64_t)addr) : -1;
}

size_t dsk_read(uint8_t *buffer, size_t size)
{
    //printf("read %d\n", size);
    return finit ? fs_read(&f, buffer, size) : -1;
}

size_t dsk_write(uint8_t *buffer, size_t size)
{
    //printf("write %d\n", size);
    return finit ? fs_write(&f, buffer, size) : -1;
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
static uint8_t cols[NCOLS] = {13, 12, 11,    10, 9, 8,    7, 6, 5,    4, 15, 14};
static uint8_t ledrows[NLEDROWS] = {20, 21, 22, 2, 3, 28, 26, 27};
static uint8_t rows[NROWS] = {16, 17, 18};

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



