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
#include "../pc/wavelan.h"

static int
wavelanpcmciareset(Ether *ether)
{
	Ctlr *ctlr;

	if((ctlr = malloc(sizeof(Ctlr))) == nil)
		return -1;

	ilock(ctlr);
	ctlr->ctlrno = ether->ctlrno;

	if (ether->ports == nil){
		ether->ports = malloc(sizeof(Devport));
		ether->ports[0].port = 0;
		ether->ports[0].size = 0;
		ether->nports= 1;
	}
	if (ether->ports[0].port==0)
		ether->ports[0].port=WDfltIOB;
	ctlr->iob = ether->ports[0].port;

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
