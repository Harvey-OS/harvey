/*
 * aoe (ata over ethernet) sd bootstrap driver, copyright © 2007 coraid
 */

#include "u.h"
#include "mem.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "sd.h"
#include "aoe.h"

enum {
	NCtlr	= 4,
	Ctlrlet = 'e',			/* first sd controller letter */

	Probeintvl	= 100,		/* ms. between probes */
	Probemax	= 8000,		/* max ms. to wait */
	Probetries	= Probemax / Probeintvl, /* max tries */
};

enum {
	/* sync with ahci.h */
	Dllba 	= 1<<0,
	Dsmart	= 1<<1,
	Dpower	= 1<<2,
	Dnop	= 1<<3,
	Datapi	= 1<<4,
	Datapi16= 1<<5,
};

enum {
	Tfree	= -1,
	Tmgmt,
};

typedef struct Ctlr Ctlr;
struct Ctlr{
	Ctlr	*next;
	SDunit	*unit;

	int	major;
	int	minor;
	uchar	ea[Eaddrlen];
	ushort	lasttag;

	ulong	vers;
	uchar	mediachange;
	uchar	flag;
	uchar	smart;
	uchar	smartrs;
	uchar	feat;

	uvlong	sectors;
	char	serial[20+1];
	char	firmware[8+1];
	char	model[40+1];
	char	ident[0x100];
};

static	Ctlr	*head;
static	Ctlr	*tail;
// static Ctlr	aoectlr[NCtlr];
// static SDev	sdevs[NCtlr];
// static int	naoectlr;

static	int	aoeether[10];

SDifc sdaoeifc;

static void
hnputs(uchar *p, ushort i)
{
	p[0] = i >> 8;
	p[1] = i;
}

static void
hnputl(uchar *p, ulong i)
{
	p[0] = i >> 24;
	p[1] = i >> 16;
	p[2] = i >> 8;
	p[3] = i;
}

static ushort
nhgets(uchar *p)
{
	return *p<<8 | p[1];
}

static ulong
nhgetl(uchar *p)
{
	return p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3];
}

static int
newtag(Ctlr *d)
{
	int t;

	for(;;){
		t = ++d->lasttag << 16;
		t |= m->ticks & 0xffff;
		switch(t) {
		case Tfree:
		case Tmgmt:
			break;
		default:
			return t;
		}
	}
}

static int
hset(Ctlr *d, Aoehdr *h, int cmd)
{
	int tag;

	memmove(h->dst, d->ea, Eaddrlen);
	hnputs(h->type, Aoetype);
	h->verflag = Aoever << 4;
	hnputs(h->major, d->major);
	h->minor = d->minor;
	h->cmd = cmd;
	hnputl(h->tag, tag = newtag(d));
	return tag;
}

static void
idmove(char *p, ushort *a, int n)
{
	int i;
	char *op, *e;

	op = p;
	for(i = 0; i < n / 2; i++){
		*p++ = a[i] >> 8;
		*p++ = a[i];
	}
	*p = 0;
	while(p > op && *--p == ' ')
		*p = 0;
	e = p;
	p = op;
	while(*p == ' ')
		p++;
	memmove(op, p, n - (e - p));
}

static ushort
gbit16(void *a)
{
	uchar *i;

	i = a;
	return i[1]<<8 | i[0];
}

static ulong
gbit32(void *a)
{
	uchar *i;
	ulong j;

	i = a;
	j  = i[3] << 24;
	j |= i[2] << 16;
	j |= i[1] << 8;
	j |= i[0];
	return j;
}

static uvlong
gbit64(void *a)
{
	uchar *i;

	i = a;
	return (uvlong)gbit32(i+4) << 32 | gbit32(a);
}

static int
ataidentify(Ctlr *c, ushort *id)
{
	vlong s;
	int i;

	i = gbit16(id+83) | gbit16(id+86);
	if(i & (1 << 10)){
		c->feat |= Dllba;
		s = gbit64(id+100);
	}else
		s = gbit32(id+60);

	idmove(c->serial, id+10, 20);
	idmove(c->firmware, id+23, 8);
	idmove(c->model, id+27, 40);

	print("sdaoe: discovered %d.%d: %s %s\n",
		c->major, c->minor, c->model, c->serial);

	c->sectors = s;
	c->mediachange = 1;
	return 0;
}

static void
identifydump(Aoeata *a)
{
	print("%E %E type=%.4ux verflag=%x error=%x %d.%d cmd=%d tag=%.8lux\n",
		a->dst, a->src, nhgets(a->type), a->verflag, a->error,
		nhgets(a->major), a->minor, a->cmd, nhgetl(a->tag));
	print("   aflag=%x errfeat=%ux scnt=%d cmdstat=%ux, lba=%d? res=%.4ux\n",
		a->aflag, a->errfeat, a->scnt, a->cmdstat, 0, nhgets(a->res));
}

static int
idpkt(Ctlr *c, Aoeata *a)
{
	memset(a, 0, sizeof *a);
	a->cmdstat = Cid;
	a->scnt = 1;
	a->lba[3] = 0xa0;
	return hset(c, a, ACata);
}

static int
chktag(int *out, int nout, int tag)
{
	int j;

	for(j = 0; j <= nout; j++)
		if(out[j] == tag)
			return 0;
	print("sdaoe: chktag: wrong aoe tag: ");
	for(j = 0; j <= nout; j++)
		print("%.8ux != %.8ux\n", out[j], tag);
	return -1;
}

static int
etherrxany(Etherpkt *p, int t)
{
	int i, n;

	for(i = 0; i < nelem(aoeether); i++)
		if(aoeether[i])
			while ((n = etherrxpkt(i, p, t)) != 0)
				if(nhgets(p->type) == Aoetype)
					return n;
	return 0;
}

static int
ethertxany(Etherpkt *p, int n)
{
	int i;

	for(i = 0; i < nelem(aoeether); i++)
		if(aoeether[i])
			ethertxpkt(i, p, n, 0);
	return 0;
}

/*
 * ignore the tag for identify.  better than ignoring
 * a response to the wrong identify request
 */
static int
identify(Ctlr *c)
{
	int tag[5], i, n;
	Aoeata *a;
	Etherpkt p;

	memset(&p, 0, sizeof p);
	a = (Aoeata*)&p;
	i = 0;
	do {
		if(i == Probetries){
			print("sdaoe: identify: timeout\n");
			return -1;
		}
		tag[i] = idpkt(c, a);
		ethertxany(&p, sizeof *a);
		memset(&p, 0, sizeof p);
next:
		n = etherrxany(&p, Probeintvl);
		if(n == 0){
			i++;
			continue;
		}
		if(nhgets(a->type) != Aoetype)
			goto next;
		if(nhgets(a->major) != c->major || a->minor != c->minor){
			print("wrong device %d.%d want %d.%d; %d\n",
				nhgets(a->major), a->minor,
				c->major, c->minor, n);
			goto next;
		}
		if(chktag(tag, i, nhgetl(a->tag)) == -1)
			goto next;
		if(a->cmdstat & 0xa9){
			print("sdaoe: ata error on identify: %2ux\n", a->cmdstat);
			return -1;
		}
	} while (a->scnt != 1);

	c->feat = 0;
	ataidentify(c, (ushort*)(a+1));
	return 0;
}

static Ctlr*
ctlrlookup(int major, int minor)
{
	Ctlr *c;

	for(c = head; c; c = c->next)
		if(c->major == major && c->minor == minor)
			break;
	return c;
}

static Ctlr*
newctlr(Etherpkt *p)
{
	int major, minor;
	Aoeqc *q;
	Ctlr *c;

	q = (Aoeqc*)p;
	if(nhgets(q->type) != Aoetype)
		return 0;
	major = nhgets(q->major);
	minor = q->minor;

	if(major == 0xffff || minor == 0xff)
		return 0;

	if(ctlrlookup(major, minor)){
		print("duplicate shelf.slot\n");
		return 0;
	}

	if((c = malloc(sizeof *c)) == 0)
		return 0;
	c->major = major;
	c->minor = minor;
	memmove(c->ea, q->src, Eaddrlen);

	if(head != 0)
		tail->next = c;
	else
		head = c;
	tail = c;
	return c;
}

static void
discover(int major, int minor)
{
	int i;
	Aoehdr *h;
	Etherpkt p;

	for(i = 0; i < nelem(aoeether); i++){
		if(aoeether[i] == 0)
			continue;
		memset(&p, 0, ETHERMINTU);
		h = (Aoehdr*)&p;
		memset(h->dst, 0xff, sizeof h->dst);
		hnputs(h->type, Aoetype);
		h->verflag = Aoever << 4;
		hnputs(h->major, major);
		h->minor = minor;
		h->cmd = ACconfig;
		ethertxpkt(i, &p, ETHERMINTU, 0);
	}
}

static int
aoeprobe(int major, int minor, SDev *s)
{
	Ctlr *ctlr;
	Etherpkt p;
	int n, i;

	n = -1;
	ctlr = nil;
	for(i = 0; n <= 0 || ctlr == nil; i += Probeintvl){
		if(i > Probemax)
			return -1;
		discover(major, minor);
		while ((n = etherrxany(&p, Probeintvl)) > 0 &&
		    (ctlr = newctlr(&p)) == nil)
			;
	}
	s->ctlr = ctlr;
	s->ifc = &sdaoeifc;
	s->nunit = 1;
	return 0;
}

static char 	*probef[32];
static int 	nprobe;

int
tokenize(char *s, char **args, int maxargs)
{
	int nargs;

	for(nargs = 0; nargs < maxargs; nargs++){
		while(*s != '\0' && strchr("\t\n ", *s) != nil)
			s++;
		if(*s == '\0')
			break;
		args[nargs] = s;
		while(*s != '\0' && strchr("\t\n ", *s) == nil)
			s++;
		if(*s != '\0')
			*s++ = 0;
	}
	return nargs;
}

static int
aoepnp0(void)
{
	int i;
	char *p, c;

	if((p = getconf("aoeif")) == nil)
		return 0;
	nprobe = tokenize(p, probef, nelem(probef));

	for(i = 0; i < nprobe; i++){
		if(strncmp(probef[i], "ether", 5) != 0)
			continue;
		c = probef[i][5];
		if(c > '9' || c < '0')
			continue;
		aoeether[c - '0'] = 1;
	}

	if((p = getconf("aoedev")) == nil)
		return 0;
	return tokenize(p, probef, nelem(probef));
}

static int
probeshelf(char *s, int *shelf, int *slot)
{
	int a, b;
	char *r;

	for(r = s + strlen(s) - 1; r > s; r--)
		if((*r < '0' || *r > '9') && *r != '.'){
			r++;
			break;
		}
	a = strtoul(r, &r, 10);
	if(*r++ != '.')
		return -1;
	b = strtoul(r, 0, 10);

	*shelf = a;
	*slot = b;
//	print("sdaoe: found shelf=%d.%d\n", a, b);
	return 0;
}

static Ctlr*
pnpprobe(SDev *sd)
{
	int shelf, slot;
	char *p;
	static int i;

	if(i >= nprobe)
		return nil;
	p = probef[i++];
	if(strlen(p) < 2)
		return nil;
	if(p[1] == '!'){
		sd->idno = p[0];
		p += 2;
	}
	if(probeshelf(p, &shelf, &slot) == -1 ||
	    aoeprobe(shelf, slot, sd) == -1 ||
	    identify(sd->ctlr) == -1)
		return nil;
	return sd->ctlr;
}

/*
 * we may need to pretend we found something
 */

static SDev*
aoepnp(void)
{
	int n, i, id;
//	char *p;
	SDev *h, *t, *s;

//	p = getconf("aoeif");
//	if (p)
//		print("aoepnp: aoeif=%s\n", p);

	if((n = aoepnp0()) == 0)
		n = 2;
	t = h = nil;
	for(i = 0; i < n; i++){
		id = Ctlrlet + i;
//		print("aoepnp: id sd%C\n", id);
		s = malloc(sizeof *s);
		if(s == nil)
			break;
		s->ctlr = 0;
		s->idno = id;
		s->ifc = &sdaoeifc;
		s->nunit = 1;
		pnpprobe(s);
		/* if pnpprobe failed, *s may contain rubbish */

		if(h)
			t->next = s;
		else
			h = s;
		t = s;
	}
	return h;
}

static int
aoeverify(SDunit *u)
{
	Ctlr *c;
	SDev *s;

	if (u == nil || u->dev == nil)
		return 0;
	s = u->dev;
	c = s->ctlr;
	if(c == nil){
		aoepnp0();
		if((s->ctlr = c = pnpprobe(s)) == nil)
			return 0;
	}
	c->mediachange = 1;
	return 1;
}

static int
aoeonline(SDunit *u)
{
	int r;
	Ctlr *c;

	if (u == nil || u->dev == nil || u->dev->ctlr == nil)
		return 0;
	c = u->dev->ctlr;
	if(c->mediachange){
		r = 2;
		c->mediachange = 0;
		u->sectors = c->sectors;
		u->secsize = 512;
	} else
		r = 1;
	return r;
}

static int
rio(Ctlr *c, Aoeata *a, int n, int scnt)
{
	int i, tag, cmd;

	for(i = 0; i < Probetries; i++){
		tag = hset(c, a, ACata);
		cmd = a->cmdstat;
		ethertxany((Etherpkt*)a, n);
		memset(a, 0, sizeof *a);
again:
		n = etherrxany((Etherpkt*)a, Probeintvl);
		if(n == 0)
			continue;
		if(nhgets(a->type) != Aoetype || nhgetl(a->tag) != tag ||
		    nhgets(a->major) != c->major || a->minor != c->minor)
			goto again;
		if(a->cmdstat & 0xa9){
			print("sdaoe: ata rio error: %2ux\n", a->cmdstat);
			return 0;
		}
		switch(cmd){
		case Crd:
		case Crdext:
			if(n - sizeof *a < scnt * 512){
				print("sdaoe: runt expect %d got %d\n",
					scnt*512 + sizeof *a, n);
				return 0;
			}
			return n - sizeof *a;
		case Cwr:
		case Cwrext:
			return scnt * 512;
		default:
			print("unknown aoe cmd %ux\n", cmd);
			break;
		}
	}
	print("sdaoe: rio timeout\n");
	return 0;
}

static void
putlba(Aoeata *a, vlong lba)
{
	uchar *c;

	c = a->lba;
	c[0] = lba;
	c[1] = lba >> 8;
	c[2] = lba >> 16;
	c[3] = lba >> 24;
	c[4] = lba >> 32;
	c[5] = lba >> 40;
}

/*
 * you'll need to loop if you want to read more than 2 sectors.  for now
 * i'm cheating and not bothering with a loop.
 */
static uchar pktbuf[1024 + sizeof(Aoeata)];

static int
aoebuild(Ctlr *c, uchar *cmd, char *data, vlong lba, int scnt)
{
	int n;
	Aoeata *a;

	memset(pktbuf, 0, sizeof pktbuf);
	a = (Aoeata*)pktbuf;
	hset(c, a, ACata);
	putlba(a, lba);

	a->cmdstat = 0x20;
	if(c->flag & Dllba){
		a->aflag |= AAFext;
		a->cmdstat |= 4;
	}else{
		a->lba[3] &= 0xf;
		a->lba[3] |= 0xe0;		/* LBA bit+obsolete 0xa0 */
	}

	n = scnt;
	if(n > 2)
		n = 2;
	a->scnt = n;

	switch(*cmd){
	case 0x2a:
		a->aflag |= AAFwrite;
		a->cmdstat |= 10;
		memmove(a+1, data, n*512);
		n = sizeof *a + n*512;
		break;
	case 0x28:
		n = sizeof *a;
		break;
	default:
		print("sdaoe: bad cmd 0x%.2ux\n", cmd[0]);
		return -1;
	}
	return n;
}

static int
aoerio(SDreq *r)
{
	int size, nsec, n;
	vlong lba;
	char *data;
	uchar *cmd;
	Aoeata *a;
	Ctlr *c;
	SDunit *unit;

	unit = r->unit;
	c = unit->dev->ctlr;
	if(r->data == nil)
		return SDok;
	cmd = r->cmd;

	lba = cmd[2]<<24 | cmd[3]<<16 | cmd[4]<<8 | cmd[5];	/* sic. */
	nsec = cmd[7]<<8 | cmd[8];
	a = (Aoeata*)pktbuf;
	data = r->data;
	r->rlen = 0;

	for(; nsec > 0; nsec -= n){
//		print("aoebuild(%2x, %p, %lld, %d)\n", *cmd, data, lba, nsec);
		size = aoebuild(c, cmd, data, lba, nsec);
		if(size < 0){
			r->status = SDcheck;
			return SDcheck;
		}
		n = a->scnt;
		r->rlen += rio(c, a, size, n);
		if(*cmd == 0x28)
			memmove(r->data, a + 1, n * 512);
		data += n * 512;
		lba += n;
	}

	r->status = SDok;
	return SDok;
}

static SDev*
aoeid(SDev* sdev)
{
	int i;
	Ctlr *c, *aec;

	for(; sdev; sdev = sdev->next){
		if(sdev->ifc != &sdaoeifc)
			continue;
		c = sdev->ctlr;
		i = 0;
		for(aec = head; aec; aec = aec->next) {
			if(c == aec)
				sdev->idno = Ctlrlet + i;
			i++;
		}
	}
	return nil;
}

SDifc sdaoeifc = {
	"aoe",

	aoepnp,
	nil,		/* legacy */
	aoeid,		/* id */
	nil,		/* enable */
	nil,		/* disable */

	aoeverify,
	aoeonline,
	aoerio,
	nil,
	nil,

	scsibio,
};
