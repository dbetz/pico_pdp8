#pragma once

void serial_putchar(int ch);
int serial_getchar();

void dsk_seek(unsigned int addr);
size_t dsk_read(void *buffer, size_t size);
size_t dsk_write(void *buffer, size_t size);

void init_pidp8i_gpio (void);
void update_led_states (uint64_t delay);
