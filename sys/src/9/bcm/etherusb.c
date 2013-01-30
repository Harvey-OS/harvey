/*
 * Kernel proxy for usb ethernet device
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
#include "../ip/ip.h"

#define	GET4(p)		((p)[3]<<24 | (p)[2]<<16 | (p)[1]<<8  | (p)[0])
#define	PUT4(p, v)	((p)[0] = (v), (p)[1] = (v)>>8, \
			 (p)[2] = (v)>>16, (p)[3] = (v)>>24)
#define	dprint	if(debug) print
#define ddump	if(0) dump

static int debug = 0;

enum {
	Bind	= 0,
	Unbind,

	SmscRxerror	= 0x8000,
	SmscTxfirst	= 0x2000,
	SmscTxlast	= 0x1000,
};

typedef struct Ctlr Ctlr;
typedef struct Udev Udev;

typedef int (Unpackfn)(Ether*, Block*);
typedef void (Transmitfn)(Ctlr*, Block*);

struct Ctlr {
	Ether*	edev;
	Udev*	udev;
	Chan*	inchan;
	Chan*	outchan;
	char*	buf;
	int	bufsize;
	int	maxpkt;
	uint	rxbuf;
	uint	rxpkt;
	uint	txbuf;
	uint	txpkt;
	QLock;
};

struct Udev {
	char	*name;
	Unpackfn *unpack;
	Transmitfn *transmit;
};
	
static Cmdtab cmds[] = {
	{ Bind,		"bind",		7, },
	{ Unbind,	"unbind",	0, },
};

static Unpackfn unpackcdc, unpackasix, unpacksmsc;
static Transmitfn transmitcdc, transmitasix, transmitsmsc;

static Udev udevtab[] = {
	{ "cdc",	unpackcdc,	transmitcdc, },
	{ "asix",	unpackasix,	transmitasix, },
	{ "smsc",	unpacksmsc,	transmitsmsc, },
	{ nil },
};

static void
dump(int c, Block *b)
{
	int s, i;

	s = splhi();
	print("%c%ld:", c, BLEN(b));
	for(i = 0; i < 32; i++)
		print(" %2.2ux", b->rp[i]);
	print("\n");
	splx(s);
}

static int
unpack(Ether *edev, Block *b, int m)
{
	Block *nb;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	ddump('?', b);
	if(m == BLEN(b)){
		etheriq(edev, b, 1);
		ctlr->rxpkt++;
		return 1;
	}
	nb = iallocb(m);
	if(nb != nil){
		memmove(nb->wp, b->rp, m);
		nb->wp += m;
		etheriq(edev, nb, 1);
		ctlr->rxpkt++;
	}else
		edev->soverflows++;
	b->rp += m;
	return 0;
}

static int
unpackcdc(Ether *edev, Block *b)
{
	int m;

	m = BLEN(b);
	if(m < 6)
		return -1;
	return unpack(edev, b, m);
}

static int
unpackasix(Ether *edev, Block *b)
{
	ulong hd;
	int m;
	uchar *wp;

	if(BLEN(b) < 4)
		return -1;
	hd = GET4(b->rp);
	b->rp += 4;
	m = hd & 0xFFFF;
	hd >>= 16;
	if(m != (~hd & 0xFFFF))
		return -1;
	m = ROUND(m, 2);
	if(m < 6 || m > BLEN(b))
		return -1;
	if((wp = b->rp + m) != b->wp && b->wp - wp < 4)
		b->wp = wp;
	return unpack(edev, b, m);
}

static int
unpacksmsc(Ether *edev, Block *b)
{
	ulong hd;
	int m;
	
	ddump('@', b);
	if(BLEN(b) < 4)
		return -1;
	hd = GET4(b->rp);
	b->rp += 4;
	m = hd >> 16;
	if(m < 6 || m > BLEN(b))
		return -1;
	if(BLEN(b) - m < 4)
		b->wp = b->rp + m;
	if(hd & SmscRxerror){
		edev->frames++;
		b->rp += m;
		if(BLEN(b) == 0){
			freeb(b);
			return 1;
		}
	}else if(unpack(edev, b, m) == 1)
		return 1;
	if((m &= 3) != 0)
		b->rp += 4 - m;
	return 0;
}

static void
transmit(Ctlr *ctlr, Block *b)
{
	Chan *c;

	ddump('!', b);
	c = ctlr->outchan;
	devtab[c->type]->bwrite(c, b, 0);
}

static void
transmitcdc(Ctlr *ctlr, Block *b)
{
	transmit(ctlr, b);
}

static void
transmitasix(Ctlr *ctlr, Block *b)
{
	int n;

	n = BLEN(b) & 0xFFFF;
	n |= ~n << 16;
	padblock(b, 4);
	PUT4(b->rp, n);
	if(BLEN(b) % ctlr->maxpkt == 0){
		padblock(b, -4);
		PUT4(b->wp, 0xFFFF0000);
		b->wp += 4;
	}
	transmit(ctlr, b);
}

static void
transmitsmsc(Ctlr *ctlr, Block *b)
{
	int n;

	n = BLEN(b) & 0x7FF;
	padblock(b, 8);
	PUT4(b->rp, n | SmscTxfirst | SmscTxlast);
	PUT4(b->rp+4, n);
	transmit(ctlr, b);
}

static void
etherusbproc(void *a)
{
	Ether *edev;
	Ctlr *ctlr;
	Chan *c;
	Block *b;

	edev = a;
	ctlr = edev->ctlr;
	c = ctlr->inchan;
	b = nil;
	if(waserror()){
		print("etherusbproc: error exit %s\n", up->errstr);
		pexit(up->errstr, 1);
		return;
	}
	for(;;){
		if(b == nil){
			b = devtab[c->type]->bread(c, ctlr->bufsize, 0);
			ctlr->rxbuf++;
		}
		switch(ctlr->udev->unpack(edev, b)){
		case -1:
			edev->buffs++;
			freeb(b);
			/* fall through */
		case 1:
			b = nil;
			break;
		}
	}
}

/*
 * bind type indev outdev mac bufsize maxpkt
 */
static void
bind(Ctlr *ctlr, Udev *udev, Cmdbuf *cb)
{
	Chan *inchan, *outchan;
	char *buf;
	uint bufsize, maxpkt;

	qlock(ctlr);
	inchan = outchan = nil;
	buf = nil;
	if(waserror()){
		free(buf);
		if(inchan)
			cclose(inchan);
		if(outchan)
			cclose(outchan);
		qunlock(ctlr);
		nexterror();
	}
	if(ctlr->buf != nil)
		cmderror(cb, "already bound to a device");
	maxpkt = strtol(cb->f[6], 0, 0);
	if(maxpkt < 8 || maxpkt > 512)
		cmderror(cb, "bad maxpkt");
	bufsize = strtol(cb->f[5], 0, 0);
	if(bufsize < maxpkt || bufsize > 32*1024)
		cmderror(cb, "bad bufsize");
	buf = smalloc(bufsize);
	inchan = namec(cb->f[2], Aopen, OREAD, 0);
	outchan = namec(cb->f[3], Aopen, OWRITE, 0);
	assert(inchan != nil && outchan != nil);
	if(parsemac(ctlr->edev->ea, cb->f[4], Eaddrlen) != Eaddrlen)
		cmderror(cb, "bad etheraddr");
	memmove(ctlr->edev->addr, ctlr->edev->ea, Eaddrlen);
	print("\netherusb %s: %E\n", udev->name, ctlr->edev->addr);
	ctlr->buf = buf;
	ctlr->inchan = inchan;
	ctlr->outchan = outchan;
	ctlr->bufsize = bufsize;
	ctlr->maxpkt = maxpkt;
	ctlr->udev = udev;
	kproc("etherusb", etherusbproc, ctlr->edev);
	poperror();
	qunlock(ctlr);
}

static void
unbind(Ctlr *ctlr)
{
	qlock(ctlr);
	if(ctlr->buf != nil){
		free(ctlr->buf);
		ctlr->buf = nil;
		if(ctlr->inchan)
			cclose(ctlr->inchan);
		if(ctlr->outchan)
			cclose(ctlr->outchan);
		ctlr->inchan = ctlr->outchan = nil;
	}
	qunlock(ctlr);
}

static long
etherusbifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *p;
	int l;

	ctlr = edev->ctlr;
	p = malloc(READSTR);
	l = 0;

	l += snprint(p+l, READSTR-l, "rxbuf: %ud\n", ctlr->rxbuf);
	l += snprint(p+l, READSTR-l, "rxpkt: %ud\n", ctlr->rxpkt);
	l += snprint(p+l, READSTR-l, "txbuf: %ud\n", ctlr->txbuf);
	l += snprint(p+l, READSTR-l, "txpkt: %ud\n", ctlr->txpkt);
	USED(l);

	n = readstr(offset, a, n, p);
	free(p);
	return n;
}

static void
etherusbtransmit(Ether *edev)
{
	Ctlr *ctlr;
	Block *b;
	
	ctlr = edev->ctlr;
	while((b = qget(edev->oq)) != nil){
		ctlr->txpkt++;
		if(ctlr->buf == nil)
			freeb(b);
		else{
			ctlr->udev->transmit(ctlr, b);
			ctlr->txbuf++;
		}
	}
}

static long
etherusbctl(Ether* edev, void* buf, long n)
{
	Ctlr *ctlr;
	Cmdbuf *cb;
	Cmdtab *ct;
	Udev *udev;

	if((ctlr = edev->ctlr) == nil)
		error(Enonexist);

	cb = parsecmd(buf, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, cmds, nelem(cmds));
	switch(ct->index){
	case Bind:
		for(udev = udevtab; udev->name; udev++)
			if(strcmp(cb->f[1], udev->name) == 0)
				break;
		if(udev->name == nil)
			cmderror(cb, "unknown etherusb type");
		bind(ctlr, udev, cb);
		break;
	case Unbind:
		unbind(ctlr);
		break;
	default:
		cmderror(cb, "unknown etherusb control message");
	}
	poperror();
	free(cb);
	return n;
}

static void
etherusbattach(Ether* edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	ctlr->edev = edev;
}

static int
etherusbpnp(Ether* edev)
{
	Ctlr *ctlr;

	ctlr = malloc(sizeof(Ctlr));
	edev->ctlr = ctlr;
	edev->irq = -1;
	edev->mbps = 100;	/* TODO: get this from usbether */
	edev->attach = etherusbattach;
	edev->transmit = etherusbtransmit;
	edev->ifstat = etherusbifstat;
	edev->ctl = etherusbctl;
	return 0;
}

void
etherusblink(void)
{
	addethercard("usb", etherusbpnp);
}
