/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum
{
	MaxEther	= 2,
	Ntypes		= 8,
};

typedef struct Ether Ether;
struct Ether {
	RWlock;				/* TO DO */
	ISAConf;			/* hardware info */
	int	ctlrno;
	int	minmtu;
	int	maxmtu;
	uchar	ea[Eaddrlen];
	void	*address;
	int	tbusy;

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*closed)(Ether*);
	void	(*detach)(Ether*);
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	long	(*ctl)(Ether*, void*, long); /* custom ctl messages */
	void	(*power)(Ether*, int);	/* power on/off */
	void	(*shutdown)(Ether*);	/* shutdown hardware before reboot */
	void	*ctlr;
	int	pcmslot;		/* PCMCIA */
	int	fullduplex;		/* non-zero if full duplex */
	int	linkchg;		/* link status changed? */
	uvlong	starttime;		/* last activity time */

	Queue*	oq;

	/* statistics */
	ulong	interrupts;
	ulong	dmarxintr;
	ulong	dmatxintr;
	ulong	promisc;
	ulong	pktsdropped;
	ulong	pktsmisaligned;
	ulong	resets;			/* after initialisation */
	ulong	bcasts;			/* broadcast pkts rcv'd */
	ulong	mcasts;			/* multicast pkts rcv'd */

	Netif;
};

extern Block* etheriq(Ether*, Block*, int);
extern void addethercard(char*, int(*)(Ether*));
extern ulong ethercrc(uchar*, int);
extern int parseether(uchar*, char*);

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
