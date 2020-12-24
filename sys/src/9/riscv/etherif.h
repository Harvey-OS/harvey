/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	MaxEther = 48,
	Ntypes = 8,
};

typedef struct Ether Ether;
struct Ether {
	ISAConf ISAConf; /* hardware info */

	int ctlrno;
	int tbdf; /* type+busno+devno+funcno */
	u8 ea[Eaddrlen];

	void (*attach)(Ether *); /* filled in by reset routine */
	void (*detach)(Ether *);
	void (*transmit)(Ether *);
	void (*interrupt)(Ureg *, void *);
	i32 (*ifstat)(Ether *, void *, i32, u32);
	i32 (*ctl)(Ether *, void *, i32); /* custom ctl messages */
	void (*power)(Ether *, int);		  /* power on/off */
	void (*shutdown)(Ether *);		  /* shutdown hardware before reboot */
	void *ctlr;

	Queue *oq;

	Netif Netif;
};

extern Block *etheriq(Ether *, Block *, int);
extern void addethercard(char *, int (*)(Ether *));
extern u32 ethercrc(unsigned char *, int);
extern int parseether(unsigned char *, char *);

#define NEXT(x, l) (((x) + 1) % (l))
#define PREV(x, l) (((x) == 0) ? (l)-1 : (x)-1)
