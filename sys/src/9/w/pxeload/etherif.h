/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct RingBuf {
	uchar owner;
	uchar unused;
	ushort len;
	uchar pkt[sizeof(Etherpkt)];
} RingBuf;

enum {
	Host = 0,      /* buffer owned by host */
	Interface = 1, /* buffer owned by card */

	Nrb = 32, /* default number of receive buffers */
	Ntb = 8,  /* default number of transmit buffers */
};

typedef struct Ether Ether;
struct Ether {
	ISAConf; /* hardware info */
	int ctlrno;
	int state;
	int tbdf;

	void (*attach)(Ether *); /* filled in by reset routine */
	void (*transmit)(Ether *);
	void (*interrupt)(Ureg *, void *);
	void (*detach)(Ether *);
	void *ctlr;

	ushort nrb;  /* number of software receive buffers */
	ushort ntb;  /* number of software transmit buffers */
	RingBuf *rb; /* software receive buffers */
	RingBuf *tb; /* software transmit buffers */

	ushort rh; /* first receive buffer belonging to host */
	ushort ri; /* first receive buffer belonging to card */

	ushort th; /* first transmit buffer belonging to host */
	ushort ti; /* first transmit buffer belonging to card */
	int tbusy; /* transmitter is busy */

	Netif;
};

extern void etherrloop(Ether *, Etherpkt *, long);
extern void addethercard(char *, int (*)(Ether *));

#define NEXT(x, l) (((x) + 1) % (l))
#define PREV(x, l) (((x) == 0) ? (l)-1 : (x)-1)
