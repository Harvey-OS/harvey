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
	"CM-56G",				/* Xircom CreditCard Modem 56 - GlobalACCESS */
	"KeepInTouch",
	"CEM56",
	0,
};

void
pcmciamodemlink(void)
{
	ISAConf isa;
	int i, j, slot;

	i = 0;
	for(;;){
		memset(&isa, 0, sizeof(isa));

		/* look for a configuration line */
		for(; i < Maxcard; i++){
			if(isaconfig("serial", i, &isa))
			if(cistrcmp(isa.type, "com") == 0)
				break;
			memset(&isa, 0, sizeof(isa));
		}

		/* default is COM2 */
		if(isa.irq == 0)
			isa.irq = 3;
		if(isa.port == 0)
			isa.port = 0x2F8;

		slot = -1;
		for(j = 0; modems[j]; j++){
			slot = pcmspecial(modems[j], &isa);
			if(slot >= 0){
				print("%s in pcmcia slot %d port 0x%lux irq %lud\n",
					modems[j], slot, isa.port, isa.irq);
				break;
			}
		}
		if(slot < 0)
			break;
	}
}	
