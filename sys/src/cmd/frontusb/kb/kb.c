/*
 * USB Human Interaction Device: keyboard and mouse.
 *
 * If there's no usb keyboard, it tries to setup the mouse, if any.
 * It should be started at boot time.
 *
 * Mouse events are converted to the format of mouse(3)
 * on mousein file.
 * Keyboard keycodes are translated to scan codes and sent to kbdfs(8)
 * on kbin file.
 *
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "hid.h"

enum
{
	Awakemsg=0xdeaddead,
	Diemsg = 0xbeefbeef,
};

enum
{
	Kbdelay = 500,
	Kbrepeat = 100,
};

typedef struct Hiddev Hiddev;
struct Hiddev
{
	Dev*	dev;		/* usb device*/
	Dev*	ep;		/* endpoint to get events */

	int	minfd;
	int	kinfd;

	Channel	*repeatc;	/* only for keyboard */

	/* report descriptor */
	int	nrep;

	/*
	 * use odd size as some devices ignore the high byte of
	 * wLength in control transfer reads.
	 */
	uchar	rep[512-1];
};

typedef struct Hidreport Hidreport;
typedef struct Hidslot Hidslot;
struct Hidslot
{
	int	valid;
	int	usage;
	int	id;
	int	oor;

	int	abs;	/* for xyz */

	int	x;
	int	y;
	int	z;

	int	b;
	int	m;

	int	w;
	int	h;
};

struct Hidreport
{
	int	ns;
	Hidslot	s[16];

	int	nk;
	uchar	k[64];

	int	o;
	uchar	*e;
	uchar	p[128];
};

/*
 * Plan 9 keyboard driver constants.
 */
enum {
	/* Scan codes (see kbd.c) */
	SCesc1		= 0xe0,		/* first of a 2-character sequence */
	SCesc2		= 0xe1,
	Keyup		= 0x80,		/* flag bit */
	Keymask		= 0x7f,		/* regular scan code bits */
};

/*
 * scan codes >= 0x80 are extended (E0 XX)
 */
#define isext(sc)	((sc) >= 0x80)

/*
 * key code to scan code; for the page table used by
 * the logitech bluetooth keyboard.
 */
static char sctab[256] = 
{
[0x00]	0x0,	0x0,	0x0,	0x0,	0x1e,	0x30,	0x2e,	0x20,
[0x08]	0x12,	0x21,	0x22,	0x23,	0x17,	0x24,	0x25,	0x26,
[0x10]	0x32,	0x31,	0x18,	0x19,	0x10,	0x13,	0x1f,	0x14,
[0x18]	0x16,	0x2f,	0x11,	0x2d,	0x15,	0x2c,	0x2,	0x3,
[0x20]	0x4,	0x5,	0x6,	0x7,	0x8,	0x9,	0xa,	0xb,
[0x28]	0x1c,	0x1,	0xe,	0xf,	0x39,	0xc,	0xd,	0x1a,
[0x30]	0x1b,	0x2b,	0x2b,	0x27,	0x28,	0x29,	0x33,	0x34,
[0x38]	0x35,	0x3a,	0x3b,	0x3c,	0x3d,	0x3e,	0x3f,	0x40,
[0x40]	0x41,	0x42,	0x43,	0x44,	0x57,	0x58,	0xe3,	0x46,
[0x48]	0xf7,	0xd2,	0xc7,	0xc9,	0xd3,	0xcf,	0xd1,	0xcd,
[0x50]	0xcb,	0xd0,	0xc8,	0x45,	0x35,	0x37,	0x4a,	0x4e,
[0x58]	0x1c,	0xcf,	0xd0,	0xd1,	0xcb,	0xcc,	0xcd,	0xc7,
[0x60]	0xc8,	0xc9,	0xd2,	0xd3,	0x56,	0xff,	0xf4,	0xf5,
[0x68]	0xd5,	0xd9,	0xda,	0xdb,	0xdc,	0xdd,	0xde,	0xdf,
[0x70]	0xf8,	0xf9,	0xfa,	0xfb,	0x0,	0x0,	0x0,	0x0,
[0x78]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0xf1,
[0x80]	0xf3,	0xf2,	0x0,	0x0,	0x0,	0xfc,	0x0,	0x0,
[0x88]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0x90]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0x98]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xa0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xa8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xb0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xb8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xc0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xc8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xd0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xd8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xe0]	0x1d,	0x2a,	0x38,	0xdb,	0xe1,	0x36,	0xb8,	0xfe,
[0xe8]	0x0,	0x0,	0x0,	0x0,	0x0,	0xf3,	0xf2,	0xf1,
[0xf0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xf8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
};

static uchar kbdbootrep[] = {
0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07,
0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01,
0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01,
0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07,
0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0,
};

static uchar ptrbootrep[] = {
0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 
0xa1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 
0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 
0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x01, 
0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 
0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03, 
0x81, 0x06, 0xc0, 0x09, 0x3c, 0x15, 0x00, 0x25, 
0x01, 0x75, 0x01, 0x95, 0x01, 0xb1, 0x22, 0x95, 
0x07, 0xb1, 0x01, 0xc0, 
};

static int debug = 0;

static int
signext(int v, int bits)
{
	int s;

	s = sizeof(v)*8 - bits;
	v <<= s;
	v >>= s;
	return v;
}

static int
getbits(uchar *p, uchar *e, int bits, int off)
{
	int v, m;

	p += off/8;
	off %= 8;
	v = 0;
	m = 1;
	if(p < e){
		while(bits--){
			if(*p & (1<<off))
				v |= m;
			if(++off == 8){
				if(++p >= e)
					break;
				off = 0;
			}
			m <<= 1;
		}
	}
	return v;
}

enum {
	Ng	= RepCnt+1,
	UsgCnt	= Delim+1,	/* fake */
	Nl	= UsgCnt+1,
	Nu	= 256,
};

static uchar*
repparse1(uchar *d, uchar *e, int g[], int l[], int c,
	void (*f)(int t, int v, int g[], int l[], int c, void *a), void *a)
{
	int z, k, t, v, i;

	while(d < e){
		v = 0;
		t = *d++;
		z = t & 3, t >>= 2;
		k = t & 3, t >>= 2;
		switch(z){
		case 3:
			d += 4;
			if(d > e) continue;
			v = d[-4] | d[-3]<<8 | d[-2]<<16 | d[-1]<<24;
			break;
		case 2:
			d += 2;
			if(d > e) continue;
			v = d[-2] | d[-1]<<8;
			break;
		case 1:
			d++;
			if(d > e) continue;
			v = d[-1];
			break;
		}
		switch(k){
		case 0:	/* main item*/
			switch(t){
			case Collection:
				memset(l, 0, Nl*sizeof(l[0]));
				i = l[Nu] | g[UsagPg]<<16;
				l[Usage] = i;
				(*f)(t, v, g, l, c, a);

				d = repparse1(d, e, g, l, v, f, a);

				l[Usage] = i;
				(*f)(CollectionEnd, v, g, l, c, a);
				continue;
			case CollectionEnd:
				return d;
			case Input:
			case Output:
			case Feature:
				if(l[UsgCnt] == 0 && l[UsagMin] != 0 && l[UsagMin] < l[UsagMax])
					for(i=l[UsagMin]; i<=l[UsagMax] && l[UsgCnt] < Nu; i++)
						l[Nl + l[UsgCnt]++] = i;
				for(i=0; i<g[RepCnt]; i++){
					l[Usage] = i < l[UsgCnt] ? l[Nl + i] : 0;
					(*f)(t, v, g, l, c, a);
				}
				break;
			}
			memset(l, 0, Nl*sizeof(l[0]));
			continue;
		case 1:	/* global item */
			if(t == Push){
				int w[Ng];
				memmove(w, g, sizeof(w));
				d = repparse1(d, e, w, l, c, f, a);
			} else if(t == Pop){
				return d;
			} else if(t < Ng){
				if(t == RepId)
					v &= 0xFF;
				else if(t == UsagPg)
					v &= 0xFFFF;
				else if(t != RepSize && t != RepCnt){
					v = signext(v, (z == 3) ? 32 : 8*z);
				}
				g[t] = v;
			}
			continue;
		case 2:	/* local item */
			if(l[Delim] != 0)
				continue;
			if(t == Delim){
				l[Delim] = 1;
			} else if(t < Delim){
				if(z != 3 && (t == Usage || t == UsagMin || t == UsagMax))
					v = (v & 0xFFFF) | (g[UsagPg] << 16);
				l[t] = v;
				if(t == Usage && l[UsgCnt] < Nu)
					l[Nl + l[UsgCnt]++] = v;
			}
			continue;
		case 3:	/* long item */
			if(t == 15)
				d += v & 0xFF;
			continue;
		}
	}
	return d;
}

/*
 * parse the report descriptor and call f for every (Input, Output
 * and Feature) main item as often as it would appear in the report
 * data packet.
 */
static void
repparse(uchar *d, uchar *e,
	void (*f)(int t, int v, int g[], int l[], int c, void *a), void *a)
{
	int l[Nl+Nu], g[Ng];

	memset(l, 0, sizeof(l));
	memset(g, 0, sizeof(g));
	repparse1(d, e, g, l, 0, f, a);
}

static int
setproto(Hiddev *f, int eid)
{
	int proto;
	Iface *iface;

	iface = f->dev->usb->ep[eid]->iface;

	/*
	 * DWC OTG controller misses some split transaction inputs.
	 * Set nonzero idle time to return more frequent reports
	 * of keyboard state, to avoid losing key up/down events.
	 */
	usbcmd(f->dev, Rh2d|Rclass|Riface, Setidle, 8<<8, iface->id, nil, 0);

	f->nrep = usbcmd(f->dev, Rd2h|Rstd|Riface, Rgetdesc, Dreport<<8, iface->id,
		f->rep, sizeof(f->rep));
	if(f->nrep > 0){
		if(debug){
			int i;

			fprint(2, "report descriptor:");
			for(i = 0; i < f->nrep; i++){
				if(i%8 == 0)
					fprint(2, "\n\t");
				fprint(2, "%#2.2ux ", f->rep[i]);
			}
			fprint(2, "\n");
		}
		proto = Reportproto;
	} else {
		switch(iface->csp){
		case KbdCSP:
			f->nrep = sizeof(kbdbootrep);
			memmove(f->rep, kbdbootrep, f->nrep);
			break;
		case PtrCSP:
			f->nrep = sizeof(ptrbootrep);
			memmove(f->rep, ptrbootrep, f->nrep);
			break;
		default:
			werrstr("no report descriptor");
			return -1;
		}
		proto = Bootproto;
	}

	/*
	 * if a HID's subclass code is 1 (boot mode), it will support
	 * setproto, otherwise it is not guaranteed to.
	 */
	if(Subclass(iface->csp) != 1)
		return 0;
 
	return usbcmd(f->dev, Rh2d|Rclass|Riface, Setproto, proto, iface->id, nil, 0);
}

static int
setleds(Hiddev* f, int, uchar leds)
{
	return usbcmd(f->dev, Rh2d|Rclass|Riface, Setreport, Reportout, 0, &leds, 1);
}

static void
hdfree(Hiddev *f)
{
	if(f->kinfd >= 0)
		close(f->kinfd);
	if(f->minfd >= 0)
		close(f->minfd);
	if(f->ep != nil)
		closedev(f->ep);
	if(f->dev != nil)
		closedev(f->dev);
	free(f);
}

static void
hdfatal(Hiddev *f, char *sts)
{
	if(sts != nil)
		fprint(2, "%s: fatal: %s\n", argv0, sts);
	else
		fprint(2, "%s: exiting\n", argv0);
	if(f->repeatc != nil)
		sendul(f->repeatc, Diemsg);
	hdfree(f);
	threadexits(sts);
}

static void
hdrecover(Hiddev *f)
{
	char err[ERRMAX];
	static QLock l;
	int i;

	if(canqlock(&l)){
		close(f->dev->dfd);
		devctl(f->dev, "reset");
		for(i=0; i<4; i++){
			sleep(500);
			if(opendevdata(f->dev, ORDWR) >= 0)
				goto Resetdone;
		}
		threadexitsall(err);
	} else {
		/* wait for reset to complete */
		qlock(&l);
	}
Resetdone:
	if(setproto(f, f->ep->id) < 0){
		rerrstr(err, sizeof(err));
		qunlock(&l);
		hdfatal(f, err);
	}
	qunlock(&l);
}

static void
putscan(Hiddev *f, uchar sc, uchar up)
{
	uchar s[2] = {SCesc1, 0};

	if(sc == 0)
		return;
	s[1] = up | sc&Keymask;
	if(isext(sc))
		write(f->kinfd, s, 2);
	else
		write(f->kinfd, s+1, 1);
}

static void
sleepproc(void* a)
{
	Channel *c = a;
	int ms;

	threadsetname("sleepproc");
	while((ms = recvul(c)) > 0)
		sleep(ms);
	chanfree(c);
}

static void
repeatproc(void* arg)
{
	Hiddev *f = arg;
	Channel *repeatc, *sleepc;
	ulong l, t;
	uchar sc;
	Alt a[3];

	repeatc = f->repeatc;
	threadsetname("repeatproc");
	
	sleepc = chancreate(sizeof(ulong), 0);
	if(sleepc != nil)
		proccreate(sleepproc, sleepc, Stack);

	a[0].c = repeatc;
	a[0].v = &l;
	a[0].op = CHANRCV;
	a[1].c = sleepc;
	a[1].v = &t;
	a[1].op = sleepc!=nil ? CHANSND : CHANNOP;
	a[2].c = nil;
	a[2].v = nil;
	a[2].op = CHANEND;

	l = Awakemsg;
	while(l != Diemsg){
		if(l == Awakemsg){
			l = recvul(repeatc);
			continue;
		}
		sc = l & 0xff;
		t = Kbdelay;
		if(alt(a) == 1){
			t = Kbrepeat;
			while(alt(a) == 1)
				putscan(f, sc, 0);
		}
	}
	if(sleepc != nil)
		sendul(sleepc, 0);
	chanfree(repeatc);
	threadexits(nil);
}

static void
stoprepeat(Hiddev *f)
{
	sendul(f->repeatc, Awakemsg);
}

static void
startrepeat(Hiddev *f, uchar sc)
{
	sendul(f->repeatc, sc);
}

static void
hidparse(int t, int f, int g[], int l[], int, void *a)
{
	Hidreport *p = a;
	Hidslot *s = &p->s[p->ns];
	int v, m;

	switch(t){
	case Input:
		if(g[RepId] != 0){
			if(p->p[0] != g[RepId]){
				p->o = 0;
				return;
			}
			if(p->o < 8)
				p->o = 8;	/* skip report id byte */
		}
		break;
	case Collection:
		if(g[RepId] != 0 && p->p[0] != g[RepId])
			return;
		if(s->valid && p->ns < nelem(p->s)-1)
			s = &p->s[++p->ns];
		memset(s, 0, sizeof(*s));
		s->usage = l[Usage];
		s->id = (p->ns+1)<<8 | g[RepId];
		return;
	case CollectionEnd:
		if(g[RepId] != 0 && p->p[0] != g[RepId])
			return;
		if(!s->valid || s->usage != l[Usage])
			return;
		/* if out of range or touchscreen finger not touching, ignore the slot */
		if(s->oor || s->usage == 0x0D0022 && s->b == 0)
			s->valid = 0;
		return;
	default:
		return;
	}
	v = getbits(p->p, p->e, g[RepSize], p->o);
	p->o += g[RepSize];

	if((f & (Fconst|Fdata)) != Fdata)
		return;

	if(debug > 1)
		fprint(2, "hidparse: t=%x f=%x usage=%x v=%x\n", t, f, l[Usage], v);

	if((l[Usage]>>16) == 0x07){	/* keycode */
		if((f & (Fvar|Farray)) == Fvar)
			if(v != 0) v = l[Usage] & 0xFF;
		if(p->nk < nelem(p->k) && v != 0)
			p->k[p->nk++] = v;
		return;
	}

	if(g[LogiMin] < 0)
		v = signext(v, g[RepSize]);
	if((f & (Fvar|Farray)) == Fvar && v >= g[LogiMin] && v <= g[LogiMax]){
		/*
		 * we use logical units below, but need the
		 * sign to be correct for mouse deltas.
		 * so if physical unit is signed but logical
		 * is unsigned, convert to signed but in logical
		 * units.
		 */
		if((f & (Fabs|Frel)) == Frel
		&& g[PhysMin] < 0 && g[PhysMax] > 0
		&& g[LogiMin] >= 0 && g[LogiMin] < g[LogiMax])
			v -= (g[PhysMax] * (g[LogiMax] - g[LogiMin])) / (g[PhysMax] - g[PhysMin]);

		switch(l[Usage]){
		default:
			return;

		case 0x090001:
		case 0x090002:
		case 0x090003:
		case 0x090004:
		case 0x090005:
		case 0x090006:
		case 0x090007:
		case 0x090008:
			m = 1<<(l[Usage] - 0x090001);
		Button:
			s->m |= m;
			s->b &= ~m;
			if(v != 0)
				s->b |= m;
			break;

		case 0x0D0032:	/* In Range */
			s->oor = !v;
			break;

		case 0x0D0042:	/* Tip Switch */
			m = 1;
			goto Button;
		case 0x0D0044:	/* Barrel Switch */
			m = 2;
			goto Button;
		case 0x0D0045:	/* Eraser */
			m = 4;
			goto Button;

		case 0x0D0048:	/* Contact width */
			s->w = v;
			break;
		case 0x0D0049:	/* Contact height */
			s->h = v;
			break;

		case 0x0D0051:	/* Contact identifier */
			s->id = v;
			break;

		case 0x010030:
			if((f & (Fabs|Frel)) == Fabs){
				v = ((vlong)(v - g[LogiMin]) << 31) / (g[LogiMax] - g[LogiMin]);
				s->abs |= 1;
			}
			s->x = v;
			break;
		case 0x010031:
			if((f & (Fabs|Frel)) == Fabs){
				v = ((vlong)(v - g[LogiMin]) << 31) / (g[LogiMax] - g[LogiMin]);
				s->abs |= 2;
			}
			s->y = v;
			break;
		case 0x010038:
			if((f & (Fabs|Frel)) == Fabs)
				s->abs |= 4;
			s->z = v;
			break;
		}
		s->valid = 1;
	}
}

static void
sethipri(void)
{
	char fn[64];
	int fd;

	snprint(fn, sizeof(fn), "/proc/%d/ctl", getpid());
	fd = open(fn, OWRITE);
	if(fd < 0)
		return;
	fprint(fd, "pri 13");
	close(fd);
}

static void
readerproc(void* a)
{
	char	err[ERRMAX], mbuf[80];
	uchar	lastk[64], uk, dk;
	int	i, c, nerrs, bpress, lastb, nlastk;
	int	abs, x, y, z, b;
	Hidreport p;
	Hidslot lasts[nelem(p.s)], *s, *l;
	Hiddev *f = a;

	threadsetname("readerproc %s", f->ep->dir);
	sethipri();

	memset(&p, 0, sizeof(p));
	memset(lasts, 0, sizeof(lasts));
	lastb = nlastk = nerrs = 0;

	for(;;){
		if(f->ep == nil)
			hdfatal(f, nil);
		if(f->ep->maxpkt < 1 || f->ep->maxpkt > sizeof(p.p))
			hdfatal(f, "hid: weird maxpkt");

		memset(p.p, 0, sizeof(p.p));
		c = read(f->ep->dfd, p.p, f->ep->maxpkt);
		if(c <= 0){
			if(c < 0)
				rerrstr(err, sizeof(err));
			else
				strcpy(err, "zero read");
			fprint(2, "%s: hid: %s: read: %s\n", argv0, f->ep->dir, err);
			if(++nerrs <= 3){
				hdrecover(f);
				continue;
			}
			hdfatal(f, err);
		}
		nerrs = 0;

		p.o = 0;
		p.e = p.p + c;
		p.ns = 0;
		memset(p.s, 0, sizeof(p.s[0]));
		repparse(f->rep, f->rep+f->nrep, hidparse, &p);
		if(p.s[p.ns].valid)
			p.ns++;

		/* handle keyboard report */
		if(p.nk != 0 || nlastk != 0){
			if(debug){
				fprint(2, "kbd: ");
				for(i = 0; i < p.nk; i++)
					fprint(2, "%#2.2ux ", p.k[i]);
				fprint(2, "\n");
			}

			if(f->kinfd < 0){
				f->kinfd = open("/dev/kbin", OWRITE);
				if(f->kinfd < 0)
					hdfatal(f, "open /dev/kbin");

				f->repeatc = chancreate(sizeof(ulong), 0);
				if(f->repeatc == nil)
					hdfatal(f, "chancreate failed");
				proccreate(repeatproc, f, Stack);
			}
	
			dk = uk = 0;
			for(i=0; i<nlastk; i++){
				if(memchr(p.k, lastk[i], p.nk) == nil){
					uk = sctab[lastk[i]];
					putscan(f, uk, Keyup);
				}
			}
			for(i=0; i<p.nk; i++){
				if(memchr(lastk, p.k[i], nlastk) == nil){
					dk = sctab[p.k[i]];
					putscan(f, dk, 0);
				}
			}
			if(uk != 0 && (dk == 0 || dk == uk))
				stoprepeat(f);
			else if(dk != 0)
				startrepeat(f, dk);

			memmove(lastk, p.k, nlastk = p.nk);
			p.nk = 0;
		}

		/* handle mouse/touchpad */
		if(p.ns == 0)
			continue;

		/* combine all the slots */
		bpress = abs = x = y = z = b = 0;
		for(i=0; i<p.ns; *l = *s, i++){
			s = &p.s[i];

			/* find the last slot of the same id */
			for(l = lasts; l->valid && l < &lasts[nelem(lasts)-1]; l++)
				if(l->usage == s->usage && l->id == s->id)
					break;
			if(l == &lasts[nelem(lasts)-1] || !l->valid)
				*l = *s;

			/* convert absolute z to relative */
			z += s->z;
			if(s->abs & 4)
				z -= l->z;

			if(debug) {
				if((s->abs & 3) == 3)
					fprint(2, "ptr[%d]: id=%x b=%x m=%x x=%f y=%f z=%d\n",
						i, s->id, s->b, s->m,
						(uint)s->x / 2147483648.0,
						(uint)s->y / 2147483648.0,
						s->z);
				else
					fprint(2, "ptr[%d]: id=%x b=%x m=%x x=%d y=%d z=%d\n",
						i, s->id, s->b, s->m, s->x, s->y, s->z);
			}

			/* map to mouse buttons */
			b |= s->b & 1;
			if(s->b & (4|8))
				b |= 2;
			if(s->b & 2)
				b |= 4;
			bpress |= s->m;

			/* X/Y are absolute? */
			if((s->abs & 3) == 3){
				/* ignore absolute position when nothing changed */
				if(s->abs == l->abs && s->x == l->x && s->y == l->y && s->b == l->b)
					continue;
				abs = 1;
				x = s->x;
				y = s->y;
			} else {
				/* everything needs to be relative */
				if((s->abs & 3) != 0 || abs)
					continue;
				x += s->x;
				y += s->y;
			}
		}

		if(bpress == 0)
			b = lastb & 7;
		if(z != 0)
			b |= z > 0 ? 8 : 16;

		if(abs || x != 0 || y != 0 || z != 0 || b != lastb){
			lastb = b;

			if(f->minfd < 0){
				f->minfd = open("/dev/mousein", OWRITE);
				if(f->minfd < 0)
					hdfatal(f, "open /dev/mousein");
			}
			seprint(mbuf, mbuf+sizeof(mbuf), "%c%11d %11d %11d", "ma"[abs], x, y, b);
			write(f->minfd, mbuf, strlen(mbuf));
		}
	}
}

static void
quirks(Hiddev *f)
{
	Dev *d;
	
	d = f->dev;
	
	/* Elecom trackball report descriptor lies by
	 * omission, failing to mention all its buttons.
	 * We patch the descriptor with a correct count
	 * which lets us parse full reports. Tested with:
	 *   Elecom HUGE (M-HT1DRBK, M-HT1URBK) */
	if(d->usb->vid == 0x056e && d->usb->did == 0x010c){
		if(f->nrep < 32
		|| f->rep[12] != 0x95
		|| f->rep[14] != 0x75
		|| f->rep[15] != 0x01
		|| f->rep[20] != 0x29
		|| f->rep[30] != 0x75)
		return;

		f->rep[13] = 8;
		f->rep[21] = 8;
		f->rep[31] = 0;
	}
}

static void
hdsetup(Dev *d, Ep *ep)
{
	Hiddev *f;

	f = emallocz(sizeof(Hiddev), 1);
	f->minfd = -1;
	f->kinfd = -1;
	incref(d);
	f->dev = d;
	if(setproto(f, ep->id) < 0){
		fprint(2, "%s: %s: setproto: %r\n", argv0, d->dir);
		goto Err;
	}
	f->ep = openep(f->dev, ep->id);
	if(f->ep == nil){
		fprint(2, "%s: %s: openep %d: %r\n", argv0, d->dir, ep->id);
		goto Err;
	}
	if(opendevdata(f->ep, OREAD) < 0){
		fprint(2, "%s: %s: opendevdata: %r\n", argv0, f->ep->dir);
		goto Err;
	}
	quirks(f);
	procrfork(readerproc, f, Stack, RFNOTEG);
	return;
Err:
	hdfree(f);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-d] devid\n", argv0);
	threadexits("usage");
}

void
threadmain(int argc, char* argv[])
{
	int i;
	Dev *d;
	Ep *ep;
	Usbdev *ud;

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	default:
		usage();
	}ARGEND;
	if(argc != 1)
		usage();
	d = getdev(*argv);
	if(d == nil)
		sysfatal("getdev: %r");
	ud = d->usb;
	for(i = 0; i < nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			continue;
		if(ep->type != Eintr || ep->dir != Ein)
			continue;
		switch(ep->iface->csp){
		case KbdCSP:
		case PtrCSP:
		case PtrNonBootCSP:
		case HidCSP:
			hdsetup(d, ep);
			break;
		}
	}
	closedev(d);
	threadexits(nil);
}
