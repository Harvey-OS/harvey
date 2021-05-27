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

enum{
	CMea = 0,
	CMmtu
};

static Cmdtab sinkcmds[] = {
	{CMea,	"ea",		2},
	{CMmtu,	"mtu",	2},
};

static long
ctl(Ether *ether, void *buf, long n)
{
	uchar ea[Eaddrlen];
	Cmdbuf *cb;
	Cmdtab *ct;
	long mtu;

	cb = parsecmd(buf, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, sinkcmds, nelem(sinkcmds));
	switch(ct->index){
	case CMea:
		if(parseether(ea, cb->f[1]) < 0)
			cmderror(cb, "invalid ea");
		memmove(ether->ea, ea, Eaddrlen);
		memmove(ether->addr, ether->ea, Eaddrlen);
		break;
	case CMmtu:
		mtu = strtol(cb->f[1], nil, 0);
		if(mtu < ETHERMINTU || mtu > 9000)
			cmderror(cb, "invalid mtu");
		ether->maxmtu = mtu;
		ether->mtu = mtu;
		break;
	}
	poperror();
	free(cb);
	return n;
}

static void
nop(Ether*)
{
}

static void
discard(Ether* ether)
{
	qflush(ether->oq);
}

static int
reset(Ether* ether)
{
	if(ether->type==nil)
		return -1;
	ether->mbps = 1000;
	ether->attach = nop;
	ether->transmit = discard;
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
