#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

/*
 *  PCMCIA modem.
 *  By default, this will set it up with the port and irq of
 *  COM2 unless a serialx=type=com line is found in plan9.ini.
 *  The assumption is that a laptop with a pcmcia will have only
 *  one com port.
 */

enum {
	Maxcard=	8,
};

static char* modems[] = {
	"IBM 33.6 Data/Fax/Voice Modem",
	"CM-56G",			/* Xircom CreditCard Modem 56 - GlobalACCESS */
	"KeepInTouch",
	"CEM56",
	"MONTANA V.34 FAX/MODEM",	/* Motorola */
	"REM10",
	"GSM/GPRS",
	"AirCard 555",
	"Gold Card Global",		/* Psion V90 Gold card */
	"Merlin UMTS Modem",		/* Novatel card */
	0,
};

void
pcmciamodemlink(void)
{
	ISAConf isa;
	int i, j, slot, com2used, usingcom2;

	i = 0;
	com2used = 0;
	for(j = 0; modems[j]; j++){
		memset(&isa, 0, sizeof(isa));

		/* look for a configuration line */
		for(; i < Maxcard; i++){
			if(isaconfig("serial", i, &isa))
			if(cistrcmp(isa.type, "com") == 0)
				break;
			memset(&isa, 0, sizeof(isa));
		}

		usingcom2 = 0;
		if (isa.irq == 0 && isa.port == 0) {
			if (com2used == 0) {
				/* default is COM2 */
				isa.irq = 3;
				isa.port = 0x2F8;
				usingcom2 = 1;
			} else
				break;
		}
		slot = pcmspecial(modems[j], &isa);
		if(slot >= 0){
			if(usingcom2)
				com2used = 1;
			if(ioalloc(isa.port, 8, 0, modems[j]) < 0)
				print("%s port %lux already in use\n", modems[j], isa.port);
			print("%s in pcmcia slot %d port 0x%lux irq %d\n",
				modems[j], slot, isa.port, isa.irq);
		}
	}
}	
