#include <stdio.h>
#include "pico/stdlib.h"

#include "nano8.h"

void serial_putchar(int ch)
{
	putchar(ch);
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
