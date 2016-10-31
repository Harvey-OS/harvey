/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
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

Ether*
archetherprobe(int cardno, int ctlrno)
{
	int i, j;
	Ether *ether;
	char buf[128], name[32];

	ether = malloc(sizeof(Ether));
	memset(ether, 0, sizeof(Ether));
	ether->ctlrno = ctlrno;
	ether->tbdf = BUSUNKNOWN;
	ether->Netif.mbps = 10;
	ether->Netif.minmtu = ETHERMINTU;
	ether->Netif.mtu = ETHERMAXTU;
	ether->Netif.maxmtu = ETHERMAXTU;

	if(cardno >= MaxEther || cards[cardno].type == nil){
		free(ether);
		return nil;
	}
	if(cards[cardno].reset(ether) < 0){
		free(ether);
		return nil;
	}

	/*
	 * IRQ2 doesn't really exist, it's used to gang the interrupt
	 * controllers together. A device set to IRQ2 will appear on
	 * the second interrupt controller as IRQ9.
	 */
	if(ether->ISAConf.irq == 2)
		ether->ISAConf.irq = 9;
	snprint(name, sizeof(name), "ether%d", ctlrno);

	/*
	 * If ether->irq is <0, it is a hack to indicate no interrupt
	 * used by ethersink.
	 */
	if(ether->ISAConf.irq >= 0)
		intrenable(ether->ISAConf.irq, ether->interrupt, ether, ether->tbdf, name);

	i = sprint(buf, "#l%d: %s: %dMbps port %#p irq %d tu %d",
		ctlrno, cards[cardno].type, ether->Netif.mbps, ether->ISAConf.port, ether->ISAConf.irq, ether->Netif.mtu);
	if(ether->ISAConf.mem)
		i += sprint(buf+i, " addr %#p", ether->ISAConf.mem);
	if(ether->ISAConf.size)
		i += sprint(buf+i, " size 0x%lX", ether->ISAConf.size);
	i += sprint(buf+i, ": %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
		ether->ea[0], ether->ea[1], ether->ea[2],
		ether->ea[3], ether->ea[4], ether->ea[5]);
	sprint(buf+i, "\n");
	print(buf);

	j = ether->Netif.mbps;
	if(j > 1000)
		j *= 10;
	for(i = 0; j >= 100; i++)
		j /= 10;
	i = (128<<i)*1024;
	netifinit(&ether->Netif, name, Ntypes, i);
	if(ether->oq == 0)
		ether->oq = qopen(i, Qmsg, 0, 0);
	if(ether->oq == 0)
		panic("etherreset %s", name);
	ether->Netif.alen = Eaddrlen;
	memmove(ether->Netif.addr, ether->ea, Eaddrlen);
	memset(ether->Netif.bcast, 0xFF, Eaddrlen);

	return ether;
}

void
archethershutdown(void)
{
	char name[32];
	int i;
	Ether *ether;

	for(i = 0; i < MaxEther; i++){
		ether = etherxx[i];
		if(ether == nil)
			continue;
		if(ether->shutdown == nil) {
			print("#l%d: no shutdown function\n", i);
			continue;
		}
		snprint(name, sizeof(name), "ether%d", i);
		if(ether->ISAConf.irq >= 0){
		//	intrdisable(ether->irq, ether->interrupt, ether, ether->tbdf, name);
		}
		(*ether->shutdown)(ether);
	}
}
