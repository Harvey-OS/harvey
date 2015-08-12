/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	Eaddrlen = 6,
	ETHERMINTU = 60,   /* minimum transmit size */
	ETHERMAXTU = 1514, /* maximum transmit size */
	ETHERHDRSIZE = 14, /* size of an ethernet header */

	MaxEther = 48,
	Ntypes = 8,
};

typedef struct Ether Ether;
struct Ether {
	ISAConf; /* hardware info */

	int ctlrno;
	int tbdf; /* type+busno+devno+funcno */
	unsigned char ea[Eaddrlen];

	void (*attach)(Ether *); /* filled in by reset routine */
	void (*detach)(Ether *);
	void (*transmit)(Ether *);
	void (*interrupt)(Ureg *, void *);
	int32_t (*ifstat)(Ether *, void *, int32_t, uint32_t);
	int32_t (*ctl)(Ether *, void *, int32_t); /* custom ctl messages */
	void (*power)(Ether *, int);		  /* power on/off */
	void (*shutdown)(Ether *);		  /* shutdown hardware before reboot */
	void *ctlr;

	int scan[Ntypes]; /* base station scanning interval */
	int nscan;	/* number of base station scanners */

	Netif;
};

typedef struct Etherpkt Etherpkt;
struct Etherpkt {
	unsigned char d[Eaddrlen];
	unsigned char s[Eaddrlen];
	unsigned char type[2];
	unsigned char data[1500];
};

extern Block *etheriq(Ether *, Block *, int);
extern void addethercard(char *, int (*)(Ether *));
extern uint32_t ethercrc(unsigned char *, int);
extern int parseether(unsigned char *, char *);

#define NEXT(x, l) (((x) + 1) % (l))
#define PREV(x, l) (((x) == 0) ? (l)-1 : (x)-1)
