#include <gba_console.h>
#include <gba_video.h>
#include <gba_sio.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>

extern void slave_main(void);
extern void master_main(void);

int configure_sio(void)
{
	// Set mode to multiplayer, 115200 bps
	REG_RCNT = 0;
	REG_SIOCNT = 0x2003;
}

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

	configure_sio();

	// ansi escape sequence to set print co-ordinates
	// /x1b[line;columnH
	iprintf("\n");
	iprintf("Link timing testing!\n\n");
	iprintf("Press A to start testing!\n\n");

	while (1) {
		VBlankIntrWait();
		scanKeys();

		if (keysDown() & KEY_A) {
			u16 is_slave = REG_SIOCNT & 4;

			if (is_slave) {
				iprintf("Detected as slave\n");
				slave_main();
			}

				master_main();
		}
	}
}
