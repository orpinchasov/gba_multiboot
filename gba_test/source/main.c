
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <stdio.h>
#include <stdlib.h>

#include "xboo.h"
#include "transfer.h"

//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------


	// the vblank interrupt must be enabled for VBlankIntrWait() to work
	// since the default dispatcher handles the bios flags no vblank handler
	// is required
	irqInit();
	irqEnable(IRQ_VBLANK);

	consoleDemoInit();

	// ansi escape sequence to set print co-ordinates
	// /x1b[line;columnH
	iprintf("\x1b[10;10HHello World!\n");

	init_transfer();
	xboo_main();

	return 0;
/*
	int i = 0;
	while (1) {
		VBlankIntrWait();
		if (i < 100) {
			transfer(0x6202);
		} else {
			transfer(0x6101);
		}
		++i;
	}
	*/
}
