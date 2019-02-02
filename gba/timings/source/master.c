#include "common.h"

#include <gba_console.h>
#include <gba_video.h>
#include <gba_sio.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>

// Send 'data' to the slave GBA and return its answer
u16 xfer(u16 data, bool verbose)
{
    REG_SIOMLT_SEND = data;

	REG_SIOCNT = SIO_115200 | SIO_MULTI;

    if (verbose)
	   iprintf("Waiting for all GBAs to become ready...\n");
	// Wait for all Gameboys to become ready
	while (!(REG_SIOCNT & SIO_SD_RDY)) {}

    if (verbose)
	   iprintf("Starting transfer\n");
	// Start transfer
	REG_SIOCNT |= SIO_START;

    if (verbose)
	   iprintf("Waiting for transfer to end\n");
	// Wait for transfer end
	while (REG_SIOCNT & SIO_START) {}
	// Wait for transfer end by interrupt
	//while (!quit) {}
    //quit = 0;

    if (verbose)
	   iprintf("send/receive = %04x%04x\n", REG_SIOMULTI0, REG_SIOMULTI1);

    return REG_SIOMULTI1;
}

// Send a handshake to the slave GBA and wait for response
void sanity_send(void)
{
    while (xfer(0x6202, true) != 0x7202) {};
}

// Measure the number of increments we manage to make during one send
void measure_loop_cycles(void)
{
	REG_SIOCNT = SIO_115200 | SIO_MULTI;

   iprintf("Waiting for all GBAs to become ready...\n");
	// Wait for all Gameboys to become ready
	while (!(REG_SIOCNT & SIO_SD_RDY)) {}

    iprintf("Starting transfer\n");
	// Start transfer
	REG_SIOCNT |= SIO_START;

    //  iprintf("Waiting for transfer to end\n");
	// Wait for transfer end
    u32 i = 0;
	while (REG_SIOCNT & SIO_START) {
        ++i;
    }

    iprintf("i = %lu\n", i);
}

void check_registers_states(void)
{
    u8 max_rcnt_states = 20;
    u8 max_siocnt_states = 20;

    u16 rcnt_states[max_rcnt_states];
    u16 siocnt_states[max_siocnt_states];

    u8 rcnt_state_index = 1;
    u8 siocnt_state_index = 1;

    REG_SIOCNT = SIO_115200 | SIO_MULTI;

    rcnt_states[0] = REG_RCNT;
    siocnt_states[0] = REG_SIOCNT;

    REG_SIOCNT |= SIO_START;

    while (REG_SIOCNT & SIO_START) {
        if ((REG_RCNT != rcnt_states[rcnt_state_index - 1]) && (rcnt_state_index < max_rcnt_states)) {
            rcnt_states[rcnt_state_index] = REG_RCNT;
            rcnt_state_index++;
        }

        if ((REG_SIOCNT != siocnt_states[siocnt_state_index - 1]) && (siocnt_state_index < max_siocnt_states)) {
            siocnt_states[siocnt_state_index] = REG_SIOCNT;
            siocnt_state_index++;
        }
    }

    iprintf("RCNT states:\n");
    for (int i = 0; i < rcnt_state_index; i++) {
        iprintf("%02x ", rcnt_states[i]);
    }

    iprintf("%02x - end\n", REG_RCNT);

    iprintf("SIOCNT states:\n");
    for (int i = 0; i < siocnt_state_index; i++) {
        iprintf("%02x ", siocnt_states[i]);
    }

    iprintf("%02x - end\n", REG_SIOCNT);
}

// Send all values from 0x0 to 0xffff and make sure they're all
// transmitted correctly to the slave
void send_all_values(void)
{
    u16 data = 0;

    for (u32 i = 0; i < 0x10000; i++) {
        xfer(data + (uint16_t)i, 1);
    }
}

void master_main(void)
{
    iprintf("Detected as master\n");

    u16 state = 3;

    while (1) {
        VBlankIntrWait();

        switch (state) {
        case 0:
            sanity_send();
            ++state;
            break;
        case 1:
            measure_loop_cycles();
            ++state;
            break;
        case 2:
            check_registers_states();
            ++state;
            break;
        case 3:
            sanity_send();
            send_all_values();
            ++state;
            break;
        default:
            break;
        }
    }
}
