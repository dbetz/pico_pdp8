//
// based on code from simple test program for 9p access to host files
// Written by Eric R. Smith, Total Spectrum Software Inc.
// Released into the public domain.
//

#include <stdlib.h>
#include <stdint.h>

extern "C" {
    #include "fs9p.h"
}

static fs9_file f;
static bool finit = false;

#include "osint.h"

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
