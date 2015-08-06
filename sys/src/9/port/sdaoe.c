/*
 * aoe sd driver, copyright © 2007 coraid
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "../port/netif.h"
#include "../port/sd.h"

#include "etherif.h"
#include "aoe.h"

extern	char	Echange[];
extern	char	Enotup[];

#define uprint(...)	snprint(up->genbuf, sizeof up->genbuf, __VA_ARGS__);

enum {
	Nctlr	= 32,
	Maxpath	= 128,
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

static char *flagname[] = {
	"llba",
	"smart",
	"power",
	"nop",
	"atapi",
	"atapi16",
};

typedef struct Ctlr Ctlr;
struct Ctlr{
	QLock qlock;

	Ctlr	*next;
	SDunit	*unit;

	char	path[Maxpath];
	Chan	*c;

	uint32_t	vers;
	unsigned char	mediachange;
	unsigned char	flag;
	unsigned char	smart;
	unsigned char	smartrs;
	unsigned char	feat;

	uint64_t	sectors;
	char	serial[20+1];
	char	firmware[8+1];
	char	model[40+1];
	char	ident[0x100];
};

void	aoeidmove(char *p, uint16_t *a, unsigned n);

static	Lock	ctlrlock;
static	Ctlr	*head;
static	Ctlr	*tail;

SDifc sdaoeifc;

static uint16_t
gbit16(void *a)
{
	uint8_t *i;

	i = a;
	return i[1] << 8 | i[0];
}

static uint32_t
gbit32(void *a)
{
	uint32_t j;
	uint8_t *i;

	i = a;
	j  = i[3] << 24;
	j |= i[2] << 16;
	j |= i[1] << 8;
	j |= i[0];
	return j;
}

static uint64_t
gbit64(void *a)
{
	uint8_t *i;

	i = a;
	return (uint64_t)gbit32(i+4)<<32 | gbit32(i);
}

static int
identify(Ctlr *c, uint16_t *id)
{
	int i;
	uint8_t oserial[21];
	uint64_t osectors, s;

	osectors = c->sectors;
	memmove(oserial, c->serial, sizeof c->serial);

	c->feat &= ~(Dllba|Dpower|Dsmart|Dnop);
	i = gbit16(id+83) | gbit16(id+86);
	if(i & (1<<10)){
		c->feat |= Dllba;
		s = gbit64(id+100);
	}else
		s = gbit32(id+60);

	i = gbit16(id+83);
	if((i>>14) == 1) {
		if(i & (1<<3))
			c->feat |= Dpower;
		i = gbit16(id+82);
		if(i & 1)
			c->feat |= Dsmart;
		if(i & (1<<14))
			c->feat |= Dnop;
	}

	aoeidmove(c->serial, id+10, 20);
	aoeidmove(c->firmware, id+23, 8);
	aoeidmove(c->model, id+27, 40);

	if((osectors == 0 || osectors != s) &&
	    memcmp(oserial, c->serial, sizeof oserial) != 0){
		c->sectors = s;
		c->mediachange = 1;
		c->vers++;
	}
	return 0;
}

/* must call with d qlocked */
static int
aoeidentify(Ctlr *d, SDunit *u)
{
	Chan *c;

	c = nil;
	if(waserror()){
		if(c)
			cclose(c);
		iprint("aoeidentify: %s\n", up->errstr);
		nexterror();
	}

	uprint("%s/ident", d->path);
	c = namec(up->genbuf, Aopen, OREAD, 0);
	c->dev->read(c, d->ident, sizeof d->ident, 0);

	poperror();
	cclose(c);

	d->feat = 0;
	d->smart = 0;
	identify(d, (uint16_t*)d->ident);

	memset(u->inquiry, 0, sizeof u->inquiry);
	u->inquiry[2] = 2;
	u->inquiry[3] = 2;
	u->inquiry[4] = sizeof u->inquiry - 4;
	memmove(u->inquiry+8, d->model, 40);

	return 0;
}

static Ctlr*
ctlrlookup(char *path)
{
	Ctlr *c;

	lock(&(&ctlrlock)->lock);
	for(c = head; c; c = c->next)
		if(strcmp(c->path, path) == 0)
			break;
	unlock(&(&ctlrlock)->lock);
	return c;
}

static Ctlr*
newctlr(char *path)
{
	Ctlr *c;

	/* race? */
	if(ctlrlookup(path))
		error(Eexist);

	if((c = malloc(sizeof *c)) == nil)
		return 0;
	kstrcpy(c->path, path, sizeof c->path);
	lock(&(&ctlrlock)->lock);
	if(head != nil)
		tail->next = c;
	else
		head = c;
	tail = c;
	unlock(&(&ctlrlock)->lock);
	return c;
}

static void
delctlr(Ctlr *c)
{
	Ctlr *x, *prev;

	lock(&(&ctlrlock)->lock);

	for(prev = 0, x = head; x; prev = x, x = c->next)
		if(strcmp(c->path, x->path) == 0)
			break;
	if(x == 0){
		unlock(&(&ctlrlock)->lock);
		error(Enonexist);
	}

	if(prev)
		prev->next = x->next;
	else
		head = x->next;
	if(x->next == nil)
		tail = prev;
	unlock(&(&ctlrlock)->lock);

	if(x->c)
		cclose(x->c);
	free(x);
}

static SDev*
aoeprobe(char *path, SDev *s)
{
	int n, i;
	char *p;
	Chan *c;
	Ctlr *ctlr;

	if((p = strrchr(path, '/')) == 0)
		error(Ebadarg);
	*p = 0;
	uprint("%s/ctl", path);
	*p = '/';

	c = namec(up->genbuf, Aopen, OWRITE, 0);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	n = uprint("discover %s", p+1);
	c->dev->write(c, up->genbuf, n, 0);
	poperror();
	cclose(c);

	for(i = 0;; i += 200){
		if(i > 8000 || waserror())
			error(Etimedout);
		tsleep(&up->sleep, return0, 0, 200);
		poperror();

		uprint("%s/ident", path);
		if(waserror())
			continue;
		c = namec(up->genbuf, Aopen, OREAD, 0);
		poperror();
		cclose(c);

		ctlr = newctlr(path);
		break;
	}

	if(s == nil && (s = malloc(sizeof *s)) == nil)
		return nil;
	s->ctlr = ctlr;
	s->ifc = &sdaoeifc;
	s->nunit = 1;
	return s;
}

static char 	*probef[32];
static int 	nprobe;

static int
pnpprobeid(char *s)
{
	int id;

	if(strlen(s) < 2)
		return 0;
	id = 'e';
	if(s[1] == '!')
		id = s[0];
	return id;
}

static SDev*
aoepnp(void)
{
	int i, id;
	char *p;
	SDev *h, *t, *s;

	if((p = getconf("aoedev")) == 0)
		return 0;
	nprobe = tokenize(p, probef, nelem(probef));
	h = t = 0;
	for(i = 0; i < nprobe; i++){
		id = pnpprobeid(probef[i]);
		if(id == 0)
			continue;
		s = malloc(sizeof *s);
		if(s == nil)
			break;
		s->ctlr = 0;
		s->idno = id;
		s->ifc = &sdaoeifc;
		s->nunit = 1;

		if(h)
			t->next = s;
		else
			h = s;
		t = s;
	}
	return h;
}

static Ctlr*
pnpprobe(SDev *sd)
{
	int j;
	char *p;
	static int i;

	if(i > nprobe)
		return 0;
	p = probef[i++];
	if(strlen(p) < 2)
		return 0;
	if(p[1] == '!')
		p += 2;

	for(j = 0;; j += 200){
		if(j > 8000){
			print("#æ: pnpprobe: %s: %s\n", probef[i-1], up->errstr);
			return 0;
		}
		if(waserror()){
			tsleep(&up->sleep, return0, 0, 200);
			continue;
		}
		sd = aoeprobe(p, sd);
		poperror();
		break;
	}
	print("#æ: pnpprobe establishes %sin %dms\n", probef[i-1], j);
	return sd->ctlr;
}


static int
aoeverify(SDunit *u)
{
	SDev *s;
	Ctlr *c;

	s = u->dev;
	c = s->ctlr;
	if(c == nil && (s->ctlr = c = pnpprobe(s)) == nil)
		return 0;
	c->mediachange = 1;
	return 1;
}

static int
aoeconnect(SDunit *u, Ctlr *c)
{
	qlock(&c->qlock);
	if(waserror()){
		qunlock(&c->qlock);
		return -1;
	}

	aoeidentify(u->dev->ctlr, u);
	if(c->c)
		cclose(c->c);
	c->c = 0;
	uprint("%s/data", c->path);
	c->c = namec(up->genbuf, Aopen, ORDWR, 0);
	qunlock(&c->qlock);
	poperror();

	return 0;
}

static int
aoeonline(SDunit *u)
{
	Ctlr *c;
	int r;

	c = u->dev->ctlr;
	r = 0;

	if((c->feat&Datapi) && c->mediachange){
		if(aoeconnect(u, c) == 0 && (r = scsionline(u)) > 0)
			c->mediachange = 0;
		return r;
	}

	if(c->mediachange){
		if(aoeconnect(u, c) == -1)
			return 0;
		r = 2;
		c->mediachange = 0;
		u->sectors = c->sectors;
		u->secsize = Aoesectsz;
	} else
		r = 1;

	return r;
}

static int
aoerio(SDreq *r)
{
	int i, count;
	uint64_t lba;
	char *name;
	uint8_t *cmd;
	int32_t (*rio)(Chan*, void*, int32_t, int64_t);
	Ctlr *c;
	SDunit *unit;

	unit = r->unit;
	c = unit->dev->ctlr;
//	if(c->feat & Datapi)
//		return aoeriopkt(r, d);

	cmd = r->cmd;
	name = unit->name;

	if(r->cmd[0] == 0x35 || r->cmd[0] == 0x91){
//		qlock(c);
//		i = flushcache();
//		qunlock(c);
//		if(i == 0)
//			return sdsetsense(r, SDok, 0, 0, 0);
		return sdsetsense(r, SDcheck, 3, 0xc, 2);
	}

	if((i = sdfakescsi(r, c->ident, sizeof c->ident)) != SDnostatus){
		r->status = i;
		return i;
	}

	switch(*cmd){
	case 0x88:
	case 0x28:
		rio = c->c->dev->read;
		break;
	case 0x8a:
	case 0x2a:
		rio = c->c->dev->write;
		break;
	default:
		print("%s: bad cmd 0x%.2ux\n", name, cmd[0]);
		r->status = SDcheck;
		return SDcheck;
	}

	if(r->data == nil)
		return SDok;

	if(r->clen == 16){
		if(cmd[2] || cmd[3])
			return sdsetsense(r, SDcheck, 3, 0xc, 2);
		lba = (uint64_t)cmd[4]<<40 | (uint64_t)cmd[5]<<32;
		lba |=   cmd[6]<<24 |  cmd[7]<<16 |  cmd[8]<<8 | cmd[9];
		count = cmd[10]<<24 | cmd[11]<<16 | cmd[12]<<8 | cmd[13];
	}else{
		lba  = cmd[2]<<24 | cmd[3]<<16 | cmd[4]<<8 | cmd[5];
		count = cmd[7]<<8 | cmd[8];
	}

	count *= Aoesectsz;

	if(r->dlen < count)
		count = r->dlen & ~0x1ff;

	if(waserror()){
		if(strcmp(up->errstr, Echange) == 0 ||
		    strcmp(up->errstr, Enotup) == 0)
			unit->sectors = 0;
		nexterror();
	}
	r->rlen = rio(c->c, r->data, count, Aoesectsz * lba);
	poperror();
	r->status = SDok;
	return SDok;
}

static char *smarttab[] = {
	"unset",
	"error",
	"threshold exceeded",
	"normal"
};

static char *
pflag(char *s, char *e, uint8_t f)
{
	uint8_t i, m;

	for(i = 0; i < 8; i++){
		m = 1 << i;
		if(f & m)
			s = seprint(s, e, "%s ", flagname[i]);
	}
	return seprint(s, e, "\n");
}

static int
aoerctl(SDunit *u, char *p, int l)
{
	Ctlr *c;
	char *e, *op;

	if((c = u->dev->ctlr) == nil)
		return 0;
	e = p+l;
	op = p;

	p = seprint(p, e, "model\t%s\n", c->model);
	p = seprint(p, e, "serial\t%s\n", c->serial);
	p = seprint(p, e, "firm	%s\n", c->firmware);
	if(c->smartrs == 0xff)
		p = seprint(p, e, "smart\tenable error\n");
	else if(c->smartrs == 0)
		p = seprint(p, e, "smart\tdisabled\n");
	else
		p = seprint(p, e, "smart\t%s\n", smarttab[c->smart]);
	p = seprint(p, e, "flag	");
	p = pflag(p, e, c->feat);
	p = seprint(p, e, "geometry %llud %d\n", c->sectors, Aoesectsz);
	return p-op;
}

static int
aoewctl(SDunit *sd, Cmdbuf *cmd)
{
	cmderror(cmd, Ebadarg);
	return 0;
}

static SDev*
aoeprobew(DevConf *c)
{
	char *p;

	p = strchr(c->type, '/');
	if(p == nil || strlen(p) > Maxpath - 11)
		error(Ebadarg);
	if(p[1] == '#')
		p++;			/* hack */
	if(ctlrlookup(p))
		error(Einuse);
	return aoeprobe(p, 0);
}

static void
aoeclear(SDev *s)
{
	delctlr((Ctlr *)s->ctlr);
}

static char*
aoertopctl(SDev *s, char *p, char *e)
{
	Ctlr *c;

	c = s->ctlr;
	return seprint(p, e, "%s aoe %s\n", s->name, c->path);
}

static int
aoewtopctl(SDev *sd, Cmdbuf *cmd)
{
	switch(cmd->nf){
	default:
		cmderror(cmd, Ebadarg);
	}
	return 0;
}

SDifc sdaoeifc = {
	"aoe",

	aoepnp,
	nil,		/* legacy */
	nil,		/* enable */
	nil,		/* disable */

	aoeverify,
	aoeonline,
	aoerio,
	aoerctl,
	aoewctl,

	scsibio,
	aoeprobew,	/* probe */
	aoeclear,	/* clear */
	aoertopctl,
	aoewtopctl,
};
