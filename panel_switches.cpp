#include <stdio.h>

#include "panel_switches.h"
#include "panel_leds.h"
#include "osint.h"

// Current switch states, as reported by the debouncing algorithm.  Set
// from the GPIO thread to control the SIMH CPU thread.
uint16_t switchstatus[NROWS];

// Time-delayed reaction to switch changes to debounce the contacts.
// This is especially important with the incandescent lamp simulation
// feature enabled since that speeds up the GPIO scanning loop, making
// it more susceptible to contact bounce.
struct switch_state {
    // switch state currently reported via switchstatus[]
    int stable_state;

    // ms the switch state has been != stable_state, or na_ms
    // if it is currently in that same state
    ms_time_t last_change;      
};
static struct switch_state gss[NROWS][NCOLS];
static const ms_time_t debounce_ms = 50;    // time switch state must remain stable

// A constant meaning "indeterminate milliseconds", used for error
// returns from ms_time() and for the case where the switch is in the
// stable state in the switch_state array.
static const ms_time_t na_ms = (ms_time_t)-1;

//// report_ss /////////////////////////////////////////////////////////
// Save given switch state ss into the exported switchstatus bitfield
// so the simulator core will see it.  (Constrast the gss matrix,
// which holds our internal view of the unstable truth.)

static void report_ss(int row, int col, int ss,
        struct switch_state* pss)
{
    pss->stable_state = ss;
    pss->last_change = na_ms;

    int mask = 1 << col;
    if (ss) switchstatus[row] |=  mask;
    else    switchstatus[row] &= ~mask;

    printf("%cSS[%d][%02d] = %d \n", gss_initted ? 'N' : 'I', row, col, ss);
}

//// debounce_switch ///////////////////////////////////////////////////
// Given the state of the switch at (row,col), work out if this requires
// a change in our exported switch state.

void debounce_switch(int row, int col, int ss, ms_time_t now_ms)
{
    struct switch_state* pss = &gss[row][col];

    if (!gss_initted) {
        // First time thru, so set this switch's module-global and
        // exported state to its defaults now that we know the switch's
        // initial state.
        report_ss(row, col, ss, pss);
    }
    else if (ss == pss->stable_state) {
        // This switch is still/again in the state we consider "stable",
        // which we are reporting in our switchstatus bitfield.  Reset
        // the debounce timer in case it is returning to its stable
        // state from a brief blip into the other state.
        pss->last_change = na_ms;
    }
    else if (pss->last_change == na_ms) {
        // This switch just left what we consider the "stable" state, so
        // start the debounce timer.
        pss->last_change = now_ms;
    }
    else if ((now_ms - pss->last_change) > debounce_ms) {
        // Switch has been in the new state long enough for the contacts
        // to have stopped bouncing: report its state change to outsiders.
        report_ss(row, col, ss, pss);
    }
    // else, switch was in the new state both this time and the one prior,
    // but it hasn't been there long enough to report it
}

