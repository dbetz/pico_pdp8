#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#include "nano8.h"
#include "panel_leds.h"
#include "panel_switches.h"

#define HLT 07402

#define LED_PIN 25

short simple_0200[] = {
07300,  // CLA CLL
01205,  // TAD 205
01206,  // TAD 206
03207,  // DCA 207
07402,  // HLT
00002,
00003
};
int simple_count_0200 = sizeof(simple_0200) / sizeof(simple_0200[0]);

short hello_0010[] = {
00207, //  STPTR,  STRNG-1     / An auto-increment register (one of eight at 10-17)
};
int hello_count_0010 = sizeof(hello_0010) / sizeof(hello_0010[0]);

short hello_0200[] = {
07300, //  HELLO,  CLA CLL       / Clear AC and Link again (needed when we loop back from tls)
01410, //          TAD I Z STPTR / Get next character, indirect via PRE-auto-increment address from the zero page
07450, //          SNA           / Skip if non-zero (not end of string)
07402, //          HLT           / Else halt on zero (end of string)
06046, //          TLS           / Output the character in the AC to the teleprinter
06041, //          TSF           / Skip if teleprinter ready for character
05205, //          JMP .-1       / Else jump back and try again
05200, //          JMP HELLO     / Jump back for the next character
00310, //  STRNG,  310           / H
00345, //          345           / e
00354, //          354           / l
00354, //          354           / l
00357, //          357           / o
00254, //          254           /,
00240, //          240           / (space)
00367, //          367           / w
00357, //          357           / o
00362, //          362           / r
00354, //          354           / l
00344, //          344           / d
00241, //          241           / !
00015, //          015           / CR
00012, //          012           / LF
00000  //          0             / End of string
};
int hello_count_0200 = sizeof(hello_0200) / sizeof(hello_0200[0]);

short os8_7750[] = {
07600,
06603,
06622,
05352,
05752,
};
int os8_count_7750 = sizeof(os8_7750) / sizeof(os8_7750[0]);

void load(int base, short data[], int count)
{
    for (int i = 0; i < count; ++i)
        mem[base + i] = data[i];
}

uint32_t sMB = 01111;   // memory buffer
int32_t sSC = 00377;

short Pause = 0;
int swStop = 0;
int swSingInst = 0;
int resumeFromInstructionLoopExit = 0;

void pauseForKey()
{
    while (serial_getchar() != '\r')
        ;
}

void show_regs(int reg)
{
    short regs[11];
    
    for (int i = 0; i < 11; ++i)
        regs[i] = 0;
    regs[reg] = 077777; // to pickup if, df, and link

    set_pidp8i_leds(
        regs [0], // pc
        regs [1], // ma
        regs [2], // mb
        regs [3], // inst
        regs [4], // acc
        regs [5], // mq
        regs [6], // ifr
        regs [7], // dfr
        regs [8], // sc
        regs [9], // intf
        regs[10]);// pause
    swap_displays();
    
    for (int i = 0; i < 8; ++i)
        printf("[%d] %08x\n", i, pdis_paint->curr[i]);
    putchar('\n');
    
    while (1) {
        if (serial_getchar() == '\r')
            break;
        update_led_states(1000);
    }
}

int main(int argc, char *argv[])
{
    stdio_init_all();
    
    init_pidp8i_gpio();
    
    bi_decl(bi_program_description("PDP-8i Simulator"));
    bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    //test_panel_led_single();
    test_panel_led_by_rows();
    
    //for (int i; i < 8; ++i) {
    //    test_panel_led_row(i);
    //    pauseForKey();
    //}
    
    for (int i = 0; i < 11; ++i)
        show_regs(i);
    
    pauseForKey();

    printf("PDP-8/I Simulator\n");
    
    if (dsk_init("df32_os8_4p.dsk") == 0) {
        printf("filesystem mounted\n");
    }

    for (int i = 0; i < MEMSIZE; ++i)
        mem[i] = HLT;
    
    load(00200, simple_0200, simple_count_0200);
    start(00200);

    while (cycl())
        ;
    
    printf("pc %04o, acc %04o, mem[0207] %04o\n", pc, acc, mem[00207] & 07777);

    set_pidp8i_leds(pc, ma, sMB, inst, acc, mq, ifr, dfr, sSC, intf, Pause);
    swap_displays();
    while (1) {
        if (serial_getchar() == '\r')
            break;
        update_led_states(1000);
    }

    load(00010, hello_0010, hello_count_0010);
    load(00200, hello_0200, hello_count_0200);
    start(00200);

    while (cycl())
        ;
    
    printf("pc %04o, acc %04o\n", pc, acc);
    
    set_pidp8i_leds(pc, ma, sMB, inst, acc, mq, ifr, dfr, sSC, intf, Pause);
    swap_displays();
    while (1) {
        if (serial_getchar() == '\r')
            break;
        update_led_states(1000);
    }

	load(07750, os8_7750, os8_count_7750);
	start(07750);
	
    while (cycl())
        ;
        
    printf("OS/8 exited\n");
    
    while (1)
        ;
    
    return 0;
}
