#pragma once

// Number of switch rows on the PiDP-8/I PCB
#define NROWS    3

// Flow-control switch states which are owned by -- that is, primarily
// modified by -- the PDP8/pidp8i module, but we can't define these
// there because we refer to them below, and not all programs that link
// to us link to that module as well.  For such programs, it's fine if
// these two flags stay 0.
extern int swStop, swSingInst;

