#pragma once

void serial_putchar(int ch);
int serial_getchar();

int dsk_init(const char *path);
int dsk_seek(unsigned int addr);
size_t dsk_read(uint8_t *buffer, size_t size);
size_t dsk_write(uint8_t *buffer, size_t size);

typedef uint32_t ms_time_t;
ms_time_t mstime();

void init_pidp8i_gpio (void);
void update_led_states (uint64_t delay);

extern int gss_initted;
void read_switches (uint64_t delay);

void debounce_switch(int row, int col, int ss, ms_time_t now_ms);



