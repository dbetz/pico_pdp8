#pragma once

#include <stdio.h>
#include "pico/stdlib.h"

#include "osint.h"

#define MEMSIZE (4096 * 8)

#define TTWAIT 10 //2000

extern short pc;
extern short acc;
extern short inst;
extern short mem[];

void start(short addr);
bool cycl(void);

// tti_iot.cpp
void tti_iot();
void tti_clear();
bool tti_idle();
bool tti_interrupt();

// tto_iot.cpp
void tto_iot();
void tto_clear();
void tto_idle();
bool tto_interrupt();

// pti_iot.cpp
void pti_iot();
void pti_clear();
void pti_idle();

// pto_iot.cpp
void pto_iot();
void pto_clear();
void pto_idle();

// dsk_iot.cpp
void dsk_iot();
void dsk_clear();

// rk8e_iot.cpp
void rk8e_iot();
