#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"../port/netif.h"

/*
 *  ethernet address stored in prom
 */
typedef struct
{
	ulong	pad0;
	ulong	byte;
	uvlong	pad1;
} Etheraddr;
#define	EPCENETPROM	EPCSWIN(Etheraddr, 0x2000)

/*
 *  SEEQ/EDLC device registers
 */
typedef struct
{
	struct
	{
	  ulong	pad;
	  ulong	byte;
	}	addr[6];	/* address */

	ulong	 pad;
	ulong	tcmd;		/* transmit command */
	ulong	 pad0;
	ulong	rcmd;		/* receive command */
	ulong	 pad1[5];
	ulong	tbaselo;	/* low bits of xmit buff base */
	ulong	 pad2;
	ulong	tbasehi;	/* high bits of xmit buff base */
	ulong	 pad3;
	ulong	tlimit;		/* xmit buffer limit */
	ulong	 pad4;
	ulong	tindex;		/* xmit buffer limit */
	ulong	 pad5;
	ulong	ttop;		/* xmit buffer top */
	ulong	 pad6;
	ulong	tbptr;		/* xmit buffer byte pointer */
	ulong	 pad7;
	ulong	tstat;		/* transmit status */
	ulong	 pad8;
	ulong	titimer;	/* transmit interrupt timer */
	ulong	 pad9[5];
	ulong	rbaselo;	/* rcv buffer base addr */
	ulong	 pad10;
	ulong	rbasehi;	/* high bits of rcv buffer base addr */
	ulong	 pad11;
	ulong	rlimit;		/* rcv buffer limit */
	ulong	 pad12;
	ulong	rindex;		/* rcv buffer index */
	ulong	 pad13;
	ulong	rtop;		/* rcv buffer top */
	ulong	 pad14;
	ulong	rbptr;		/* rcv buffer byte pointer */
	ulong	 pad15;
	ulong	rstat;		/* rcv status */
	ulong	 pad16;
	ulong	ritimer;	/* rcv interrupt timer */
} EDLCdev;
#define EPCEDLC	EPCSWIN(EDLCdev, 0xa200)

/*
 *  LXT901 device registers
 */
typedef struct {
	ulong	 pad0;
	ulong	stat;		/* some LXT901 pins + SQE ctrl (r/w) */
	ulong	 pad1;
	ulong	collisions;	/* total collisions -- 16 bits (r) */
	uchar	 pad2[7];
	uchar	loopback;	/* loopback enable -- 1 bit (w) */
	ulong	 pad3;
	ulong	edlcself;	/* EDLC self rcv enable -- 1 bit (w) */
} LXTdev;
#define EPCLXT	EPCSWIN(LXTdev, 0x8000)

enum
{
	/* transmit command bits */
	Tigood=		1<<3,		/* interrupt on good xmit */
	Ti16tries=	1<<2,		/* interrupt on 16 retries */
	Ticoll=		1<<1,		/* interrupt on collision */
	Tiunder=	1<<0,		/* interrupt on underflow */

	/* receive command bits */
	Rmulti=		3<<6,		/* recv station/broadcast/multicast */
	Rnormal=	2<<6,		/* recv station/broadcast */
	Rall=		1<<6,		/* receive all frames */
	Rigood=		1<<5,	 	/* interrupt on good frames */
	Riend=		1<<4,		/* interrupt on end of frame */
	Rishort=	1<<3,		/* interrupt on short frame */
	Ridrbl=		1<<2,		/* interrupt on dribble error */
	Ricrc=		1<<1,		/* interrupt on CRC error */
	Riover=		1<<0,		/* interrupt on overflow error */

	Renab=		(Rigood|Riend|Rishort|Ridrbl|Ricrc),

	/* receive status bits */
	Rgood=		1<<5,		/* good frame */
	Rend=		1<<4,		/* end of frame */
	Rshort=		1<<3,		/* short frame */
	Rdrbl=		1<<2,		/* dribble error */
	Rcrc=		1<<1,		/* CRC error */
	Rover=		1<<0,		/* overflow error */

	/* interrupt level for ether */
	ILenet=		0x60,

	/* manifest constants */
	logNxmt=	8,
	Nxmt=		1<<logNxmt,
	Tmask=		Nxmt-1,
	logNrcv=	8,
	Nrcv=		1<<logNrcv,
	Rmask=		Nrcv-1,
	Ntypes=		8,

	/* hold off values */
	ho800us=	0x100,	/* 800 us hold-off */
	ho1500us=	0x080,	/* 1500 us hold-off */
	ho2500us=	0x000,	/* 2500 us hold-off */
};

#define RSUCC(i)	(((i)+1)&Rmask)
#define RPREV(i)	(((i)-1)&Rmask)
#define TSUCC(i)	(((i)+1)&Tmask)
#define TPREV(i)	(((i)-1)&Tmask)

/*
 *  a hardware packet buffer
 */
typedef struct
{
	uchar	tlen[2];	/* transmit length */
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
	uchar	pad1[2043-ETHERMAXTU];
	uchar	stat;
	uchar	rlen[2];	/* receive length */
} Pbuf;

struct Ether
{
	uchar	ea[6];

	int	rindex;		/* first rcv buffer owned by hardware */
	int	rtop;		/* first rcv buffer owned by software */
	int	tindex;		/* first rcv buffer owned by hardware */
	int	ttop;		/* first rcv buffer owned by software */

	Pbuf	*tbuf;		/* transmit buffers */
	Pbuf	*rbuf;		/* receive buffers */

	QLock	tlock;		/* lock for grabbing transmitter queue */
	Rendez	tr;		/* wait here for free xmit buffer */

	Netif;
} ether;


/*
 *  The dance in this code is very dangerous to change.  Do not
 *  change the order of any of the labeled steps.  This should run splhi.
 */
static void
etherhardreset(void)
{
	EDLCdev *edlc = EPCEDLC;
	LXTdev *lxt = EPCLXT;
	EPCmisc *misc = EPCMISC;
	ulong x, i;

	/* step 1: isolate from ether */
	lxt->loopback = 1;
	x = lxt->loopback; USED(x);

	/* step 2: shut off transmitter
	while(edlc->ttop != edlc->tindex)
		edlc->ttop = edlc->tindex;
	*/
	ether.ttop = edlc->ttop;
	ether.tindex =  edlc->ttop;

	/* step 3: reset edlc */
	misc->set = 0x200;
	x = misc->reset; USED(x);
	delay(1);	/* 1ms but 10micros is enough */
	misc->clr = 0x200;
	x = misc->reset; USED(x);

	/* step 4: enable transmitter interrupts */
	edlc->tcmd = Tigood | Ti16tries | Ticoll | Tiunder;

	/* step 5: set address from prom, start receiver,
	 * and reset receive pointer
	 */
	for(i = 0; i < Netheraddr; i++)
		edlc->addr[i].byte = EPCENETPROM[5-i].byte & 0xff;

	if(ether.prom)
		edlc->rcmd = Renab | Rall;
	else
		edlc->rcmd = Renab | Rnormal;

	ether.rindex = edlc->rindex;
	ether.rtop = edlc->rtop = RPREV(ether.rindex);

	/* step 6: attach to ether */
	lxt->loopback = 0;

	print("etherhardreset: rtop collision\n");
}

static int
isoutbuf(void *arg)
{
	EDLCdev *edlc = arg;

	ether.tindex = edlc->tindex;
	return TSUCC(ether.ttop) != ether.tindex;
}

void
etherintr(void)
{
	EDLCdev *edlc = EPCEDLC;
	EPCmisc *misc = EPCMISC;
	Netfile *f, **fp;
	Pbuf *p;
	int x;
	ushort t;

	while(edlc->rindex != ether.rindex){
		p = &ether.rbuf[ether.rindex];

		/* statistics */
		if(p->stat & (Rshort|Rdrbl|Rcrc|Rover)){
			if(p->stat & (Rdrbl|Rcrc))
				ether.crcs++;
			if(p->stat & Rover)
				ether.overflows++;
			if(p->stat & Rshort)
				ether.frames++;
		}
		if(p->stat & Rgood){
			ether.inpackets++;
			x = (p->rlen[0]<<8) | p->rlen[1];
			t = (p->type[0]<<8) | p->type[1];
			for(fp = ether.f; fp < &ether.f[Ntypes]; fp++){
				f = *fp;
				if(f == 0)
					continue;
				if(f->type == t || f->type < 0)
					qproduce(f->in, p->d, x);
			}
		}

		edlc->rtop = ether.rindex;
		ether.rindex = RSUCC(ether.rindex);
	}
	/*
	 *  because of a chip bug, we have to reset if rtop
	 *  and rindex get too close.
	 */
	x = edlc->rtop - edlc->rindex;
	if(x < 0)
		x += Nrcv;
	if(x <= 64)
		etherhardreset();

	/* get output going if we're blocked */
	if(isoutbuf(edlc))
		wakeup(&ether.tr);

	/* reenable holdoff and EPC interrupts */
	misc->clr = 0x10;
	misc->set = 0x10;
	epcenable(EIenet);
}

/*
 *  turn promiscuous mode on/off
 */
static void
promiscuous(void*, int on)
{
	EDLCdev *edlc = EPCEDLC;

	if(on)
		edlc->rcmd = Renab | Rall;
	else
		edlc->rcmd = Renab | Rnormal;
}

static void
etherreset(void)
{
	EDLCdev *edlc = EPCEDLC;
	EPCmisc *misc = EPCMISC;
	ulong x, i;
	uchar *p;

	/* setup xmit buffers (pointers on chip assume KSEG0) */
	edlc->tlimit = logNxmt - 1;
	ether.tbuf = xspanalloc(Nxmt*sizeof(Pbuf), 512*1024, 0);
	x = ((ulong)ether.tbuf) & ~KSEGM;
	edlc->tbasehi = 0;
	edlc->tbaselo = x;

	/* setup rcv buffers (pointers on chip assume KSEG0) */
	edlc->rlimit = logNrcv - 1;
	ether.rbuf = xspanalloc(Nrcv*sizeof(Pbuf), 512*1024, 0);
	x = ((ulong)ether.rbuf) & ~KSEGM;
	edlc->rbasehi = 0;
	edlc->rbaselo = x;

	/* install interrupt handler */
	sethandler(ILenet, etherintr);
	setleveldest(ILenet, 0, &EPCINTR->enetdest);
	epcenable(EIenet);

	/* turn off receive */
	edlc->rcmd = 0;

	/* stop transmitter, we can't change tindex so we change ttop */
	while(edlc->ttop != edlc->tindex)
		edlc->ttop = edlc->tindex;
	ether.tindex = ether.ttop = edlc->ttop;

	/* enable transmitter interrupts */
	edlc->tcmd = Tigood | Ti16tries | Ticoll | Tiunder;

	/* set address from prom, start receiver, and reset receive pointer */
	for(i = 0; i < Netheraddr; i++){
		ether.ea[i] = EPCENETPROM[5-i].byte & 0xff;
		edlc->addr[i].byte = ether.ea[i];
	}
	ether.rindex = edlc->rindex;
	ether.rtop = edlc->rtop = RPREV(ether.rindex);

	edlc->rcmd = Renab | Rnormal;

	/* set hold off timer value, and enable its interrupts (pulse) */
	misc->set = ho800us;
	misc->clr = (~ho800us)&0x180;
	misc->clr = 0x10;
	misc->set = 0x10;

	/* general network interface structure */
	netifinit(&ether, "ether0", Ntypes, 32*1024);
	ether.alen = 6;
	memmove(ether.addr, ether.ea, Eaddrlen);
	memset(ether.bcast, 0xFF, Eaddrlen);
	ether.promiscuous = promiscuous;
	ether.arg = &ether;

	p = ether.ea;
	print("etheraddr: %ux:%ux:%ux:%ux:%ux:%ux\n", p[0], p[1], p[2],
		p[3], p[4], p[5]);
}

static Chan*
etherattach(char *spec)
{
	return devattach('l', spec);
}

static int
etherwalk(Chan *c, char *name)
{
	return netifwalk(&ether, c, name);
}

static Chan*
etheropen(Chan *c, int omode)
{
	return netifopen(&ether, c, omode);
}

static void
ethercreate(Chan*, char*, int, ulong)
{
}

static void
etherclose(Chan *c)
{
	netifclose(&ether, c);
}

static long
etherread(Chan *c, void *buf, long n, vlong off)
{
	ulong offset = off;
	return netifread(&ether, c, buf, n, offset);
}

static Block*
etherbread(Chan *c, long n, ulong offset)
{
	return netifbread(&ether, c, n, offset);
}

static int
etherloop(Etherpkt *p, long n)
{
	int s, different;
	ushort t;
	Netfile *f, **fp;

	different = memcmp(p->d, ether.ea, sizeof(ether.ea));
	if(different && memcmp(p->d, ether.bcast, sizeof(p->d)))
		return 0;

	s = splhi();
	t = (p->type[0]<<8) | p->type[1];
	for(fp = ether.f; fp < &ether.f[Ntypes]; fp++) {
		f = *fp;
		if(f == 0)
			continue;
		if(f->type == t || f->type < 0)
			if(qproduce(f->in, p->d, n) < 0)
				ether.soverflows++;
	}
	splx(s);
	return !different;
}

static long
etherwrite(Chan *c, void *buf, long n, vlong)
{
	Pbuf *p;
	EDLCdev *edlc = EPCEDLC;

	if(n > ETHERMAXTU)
		error(Ebadarg);

	/* etherif.c handles structure */
	if(NETTYPE(c->qid.path) != Ndataqid)
		return netifwrite(&ether, c, buf, n);

	/* we handle data */
	if(etherloop(buf, n))
		return n;

	qlock(&ether.tlock);
	if(waserror()) {
		qunlock(&ether.tlock);
		nexterror();
	}

	tsleep(&ether.tr, isoutbuf, edlc, 10000);
	if(!isoutbuf(edlc)){
		print("ether transmitter jammed\n");
	} else {
		ether.outpackets++;
		p = &ether.tbuf[ether.ttop];
		memmove(p->d, buf, n);
		if(n < 60) {
			memset(p->d+n, 0, 60-n);
			n = 60;
		}
		memmove(p->s, ether.ea, sizeof(ether.ea));
		p->tlen[0] = n;
		p->tlen[1] = n>>8;
		ether.ttop = TSUCC(ether.ttop);
		edlc->ttop = ether.ttop;
	}
	qunlock(&ether.tlock);
	poperror();
	return n;
}

static long
etherbwrite(Chan *c, Block *bp, ulong offset)
{
	return devbwrite(c, bp, offset);
}

static void
etherremove(Chan*)
{
}

static void
etherstat(Chan *c, char *dp)
{
	netifstat(&ether, c, dp);
}

static void
etherwstat(Chan *c, char *dp)
{
	netifwstat(&ether, c, dp);
}

int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

Dev etherdevtab = {
	'l',
	"ether",

	etherreset,
	devinit,
	etherattach,
	devclone,
	etherwalk,
	etherstat,
	etheropen,
	ethercreate,
	etherclose,
	etherread,
	etherbread,
	etherwrite,
	etherbwrite,
	etherremove,
	etherwstat,
};
