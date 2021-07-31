#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

#include "ether.h"

enum {
	SelectWindow	= 0x01,		/* SelectWindow command */

	Command		= 0x0E,		/* all windows */
	Status		= 0x0E,

	ManufacturerID	= 0x00,		/* window 0 */
	ProductID	= 0x02,
	ConfigControl	= 0x04,
	AddressConfig	= 0x06,
	ResourceConfig	= 0x08,
	EEPROMcmd	= 0x0A,
	EEPROMdata	= 0x0C,
};

#define COMMAND(port, cmd, a)	outs(port+Command, ((cmd)<<11)|(a))

extern int ether509reset(Ctlr*);

static int
reset(Ctlr *ctlr)
{
	int slot;
	int port;
	ushort x;

	if(ctlr->card.irq == 0)
		ctlr->card.irq = 10;
	if(ctlr->card.port == 0)
		ctlr->card.port = 0x240;
	port = ctlr->card.port;

	slot = pcmspecial("3C589", &ctlr->card);
	if(slot < 0)
		return -1;

	/* set Window 0 configuration registers */
	COMMAND(port, SelectWindow, 0);

	/* ROM size & base - must be set before we can access ROM */
	/* transceiver type (for now always 10baseT) */
	x = ins(port + AddressConfig);
	outs(port + AddressConfig, x & 0x20);

	/* IRQ must be 3 on 3C589 */
	x = ins(port + ResourceConfig);
	outs(port + ResourceConfig, 0x3f00 | (x&0xfff));

	/* move product ID to register */
	while(ins(port+EEPROMcmd) & 0x8000)
		;
	outs(port+EEPROMcmd, (2<<6)|3);
	while(ins(port+EEPROMcmd) & 0x8000)
		;
	x = ins(port+EEPROMdata);
	outs(port + ProductID, x);

	if(ether509reset(ctlr) < 0){
		pcmspecialclose(slot);
		return -1;
	}
	return 0;
}

void
ether589link(void)
{
	addethercard("3C589", reset);
}
