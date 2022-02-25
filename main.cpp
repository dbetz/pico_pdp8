#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#include "nano8.h"

#define HLT 07402

#define LED_PIN 25

short simple_0200[] = {
07300,
01205,
01206,
03207,
07402,
00002,
00003
};
int simple_count_0200 = sizeof(simple_0200) / sizeof(simple_0200[0]);

short hello_0010[] = {
00207, //  STPTR,    STRNG-1     / An auto-increment register (one of eight at 10-17)
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

void load(int base, short data[], int count)
{
	for (int i = 0; i < count; ++i)
	    mem[base + i] = data[i];
}

int main(int argc, char *argv[])
{
    stdio_init_all();
    
    bi_decl(bi_program_description("PDP-8i Simulator"));
    bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    sleep_ms(4000);

    printf("PDP-8i Simulator\n");
    
    for (int i = 0; i < MEMSIZE; ++i)
		mem[i] = HLT;
	
    while (1) {
    
        load(00200, simple_0200, simple_count_0200);
        start(00200);
    
        while (cycl())
            ;
        
        printf("pc %04o, acc %04o, mem[0207] %04o\n", pc, acc, mem[00207] & 07777);
    
        load(00010, hello_0010, hello_count_0010);
        load(00200, hello_0200, hello_count_0200);
        start(00200);
    
        while (cycl())
            ;
        
        printf("pc %04o, acc %04o\n", pc, acc);
    }
    
    return 0;
}
