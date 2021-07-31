/* Bitsy pcmcia code for wavelan.c */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"
#include "wavelan.h"

static int
wavelanpcmciareset(Ether *ether)
{
	Ctlr *ctlr;

	if((ctlr = malloc(sizeof(Ctlr))) == nil)
		return -1;

	ilock(ctlr);
	ctlr->ctlrno = ether->ctlrno;

	if (ether->port==0)
		ether->port=WDfltIOB;
	ctlr->iob = ether->port;

	if(wavelanreset(ether, ctlr) < 0){
		iunlock(ctlr);
		free(ctlr);
		ether->ctlr = nil;
		return -1;
	}
	iunlock(ctlr);
	return 0;
}

void
etherwavelanlink(void)
{
	addethercard("wavelan", wavelanpcmciareset);
}
