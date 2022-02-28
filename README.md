# pico_pdp8
PDP-8 Simulator for the Raspberry Pi Pico and the PiDP8/I Front Panel

This code is heavily based on Oscar Vermeulen's PiDP8/I software and it's intent is to
allow a Raspberry Pi Pico module to replace the Raspberry Pi in Oscar's design. It is
also based on Ian Schofield's PDP-8 simulator for the Pimeroni Tiny 2040. I switched to
the Raspberry Pi Pico because it exposes enough pins to interface to the PiDP8/I front
panel.

This code is a work in progress. The basic PDP-8 simulator is working thanks to Ian's
code but the interface to the PiDP8/I front panel is in progress. Also, only the TTY
IOT instructions are implemented so it will not run OS/8 at present. My priority is to
get the front panel interface working before diving into the disk interface.

The serial filesystem code that supports the DF32 drive emulation is based on code
by Eric Smith.

Anyway, my thanks to Oscar, Ian, and Eric for making their code available!

David Betz
