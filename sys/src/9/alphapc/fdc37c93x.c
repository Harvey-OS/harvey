#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 * SMC FDC37C93x Plug and Play Compatible Ultra I/O Controller.
 */
enum {					/* I/O Ports */
	Config		= 0x370,	/* could also be 0x3F0 */

	Index		= 0,
	Data		= 1,
};

static int fddregs[] = {
	0x30,
	0x60, 0x61,
	0x70,
	0x74,
	0xF0,
	0xF1,
	0xF2,
	0xF4,
	0xF5,
	0,
};

#define OUTB(p, d)	outb(p, d); microdelay(10);

void
fdc37c93xdump(void)
{
	int config, i, x;

	config = Config;

	OUTB(config, 0x55);
	OUTB(config, 0x55);

	OUTB(config+Index, 0x20);
	x = inb(config+Data);
	print("fdc37c93x: Device ID 0x%2.2uX\n", x);
	OUTB(config+Index, 0x22);
	x = inb(config+Data);
	print("fdc37c93x: Power/Control 0x%2.2uX\n", x);

	OUTB(config+Index, 0x07);
	OUTB(config+Data, 0);
	for(i = 0; fddregs[i]; i++){
		OUTB(config+Index, fddregs[i]);
		x = inb(config+Data);
		print("FDD%2.2uX: 0x%2.2uX\n", fddregs[i], x);
	}

	OUTB(config+Index, 0x70);
	OUTB(config+Data, 0x06);
	OUTB(config+Index, 0x74);
	OUTB(config+Data, 0x02);
	OUTB(config+Index, 0x30);
	OUTB(config+Data, 0x01);

	OUTB(config, 0xAA);
}
