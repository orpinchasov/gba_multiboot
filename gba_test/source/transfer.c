#include <gba_console.h>
#include <gba_interrupt.h>
#include <gba_video.h>
#include <gba_systemcalls.h>
#include <gba_sio.h>
#include <stdio.h>

#include "transfer.h"
#include "xboo.h"

#define SIO_SD_RDY (1<<3)

volatile int quit = 0;

void SerialInterrupt()
{
	quit = 1;
}

void init_transfer()
{
    irqEnable(IRQ_SERIAL);

	irqSet(IRQ_SERIAL, SerialInterrupt);
}

void transfer(u16 data)
{
	REG_RCNT = R_MULTI;
	REG_SIOMLT_SEND = data;

	//iprintf("Setting SIOCNT\n");
	REG_SIOCNT = SIO_115200 | SIO_MULTI | SIO_IRQ;

	//iprintf("Waiting for all GBAs to become ready...\n");
	// Wait for all Gameboys to become ready
	while (!(REG_SIOCNT & SIO_SD_RDY)) {}

	//iprintf("Starting transfer\n");
	// Start transfer
	REG_SIOCNT |= SIO_START;

	//iprintf("Waiting for transfer to end\n");
	// Wait for transfer end
	//while (REG_SIOCNT & SIO_START) {}
	// Wait for transfer end by interrupt
	while (!quit) {}
    quit = 0;

	iprintf("send/receive = %04x%04x\n", REG_SIOMLT_SEND, REG_SIOMULTI1);

    rec[1] = (unsigned char)(REG_SIOMULTI1 & 0xFF);
    rec[0] = (unsigned char)((REG_SIOMULTI1 >> 8) & (0xFF));

    // TODO: This seems unnecessary, but I'm not sure.
    //VBlankIntrWait();
}
