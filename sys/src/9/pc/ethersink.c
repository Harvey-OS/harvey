/*
 * An ethernet /dev/null.
 * Useful as a bridging target with ethernet-based VPN.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

static long
ctl(Ether *ether, void *buf, long n)
{
	uchar ea[Eaddrlen];
	Cmdbuf *cb;

	cb = parsecmd(buf, n);
	if(cb->nf >= 2
	&& strcmp(cb->f[0], "ea")==0
	&& parseether(ea, cb->f[1]) == 0){
		free(cb);
		memmove(ether->ea, ea, Eaddrlen);
		memmove(ether->addr, ether->ea, Eaddrlen);
		return 0;
	}
	free(cb);
	error(Ebadctl);
	return -1;	/* not reached */
}

static void
nop(Ether*)
{
}

static int
reset(Ether* ether)
{
	uchar ea[Eaddrlen];

	if(ether->type==nil)
		return -1;
	memset(ea, 0, sizeof ea);
	ether->mbps = 1000;
	ether->attach = nop;
	ether->transmit = nop;
	ether->irq = -1;
	ether->interrupt = nil;
	ether->ifstat = nil;
	ether->ctl = ctl;
	ether->promiscuous = nil;
	ether->multicast = nil;
	ether->arg = ether;
	return 0;
}

void
ethersinklink(void)
{
	addethercard("sink", reset);
}
