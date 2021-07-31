#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

/*
 *  AT&T Keep In Touch modem
 *
 *  By default, this will set it up with the port and irq of
 *  COM2 unless a serialx=type=com line is found in plan9.ini.
 *  The assumption is that a laptop with a pcmcia will have only
 *  one com port.
 */

enum {
	Maxcard=	8,
};

void
pcmkitlink(void)
{
	ISAConf isa;
	int i, slot;

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

		/* default is COM1 */
		if(isa.irq == 0)
			isa.irq = 3;
		if(isa.port == 0)
			isa.port = 0x2F8;

		slot = pcmspecial("KeepInTouch", &isa);
		if(slot < 0)
			break;
print("pcmcia slot %d Keep in touch modem\n", slot);
	}
}	
