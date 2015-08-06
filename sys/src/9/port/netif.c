/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/netif.h"

static int netown(Netfile*, char*, int);
static int openfile(Netif*, int);
static char* matchtoken(char*, char*);
static char* netmulti(Netif*, Netfile*, uint8_t*, int);
static int parseaddr(uint8_t*, char*, int);

/*
 *  set up a new network interface
 */
void
netifinit(Netif *nif, char *name, int nfile, uint32_t limit)
{
	strncpy(nif->name, name, KNAMELEN-1);
	nif->name[KNAMELEN-1] = 0;
	nif->nfile = nfile;
	nif->f = malloc(nfile*sizeof(Netfile*));
	if(nif->f == nil)
		panic("netifinit: no memory");
	memset(nif->f, 0, nfile*sizeof(Netfile*));
	nif->limit = limit;
}

/*
 *  generate a 3 level directory
 */
static int
netifgen(Chan *c, char* j, Dirtab *vp, int n, int i, Dir *dp)
{
	Proc *up = externup();
	Qid q;
	Netif *nif = (Netif*)vp;
	Netfile *f;
	int t;
	int perm;
	char *o;

	q.type = QTFILE;
	q.vers = 0;

	/* top level directory contains the name of the network */
	if(c->qid.path == 0){
		switch(i){
		case DEVDOTDOT:
			q.path = 0;
			q.type = QTDIR;
			devdir(c, q, ".", 0, eve, 0555, dp);
			break;
		case 0:
			q.path = N2ndqid;
			q.type = QTDIR;
			strcpy(up->genbuf, nif->name);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			break;
		default:
			return -1;
		}
		return 1;
	}

	/* second level contains clone plus all the conversations */
	t = NETTYPE(c->qid.path);
	if(t == N2ndqid || t == Ncloneqid || t == Naddrqid
	|| t == Nstatqid || t == Nifstatqid || t == Nmtuqid){
		switch(i) {
		case DEVDOTDOT:
			q.type = QTDIR;
			q.path = 0;
			devdir(c, q, ".", 0, eve, DMDIR|0555, dp);
			break;
		case 0:
			q.path = Ncloneqid;
			devdir(c, q, "clone", 0, eve, 0666, dp);
			break;
		case 1:
			q.path = Naddrqid;
			devdir(c, q, "addr", 0, eve, 0666, dp);
			break;
		case 2:
			q.path = Nstatqid;
			devdir(c, q, "stats", 0, eve, 0444, dp);
			break;
		case 3:
			q.path = Nifstatqid;
			devdir(c, q, "ifstats", 0, eve, 0444, dp);
			break;
		case 4:
			q.path = Nmtuqid;
			devdir(c, q, "mtu", 0, eve, 0444, dp);
			break;
		default:
			i -= 5;
			if(i >= nif->nfile)
				return -1;
			if(nif->f[i] == 0)
				return 0;
			q.type = QTDIR;
			q.path = NETQID(i, N3rdqid);
			snprint(up->genbuf, sizeof up->genbuf, "%d", i);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			break;
		}
		return 1;
	}

	/* third level */
	f = nif->f[NETID(c->qid.path)];
	if(f == 0)
		return 0;
	if(*f->owner){
		o = f->owner;
		perm = f->mode;
	} else {
		o = eve;
		perm = 0666;
	}
	switch(i){
	case DEVDOTDOT:
		q.type = QTDIR;
		q.path = N2ndqid;
		strcpy(up->genbuf, nif->name);
		devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
		break;
	case 0:
		q.path = NETQID(NETID(c->qid.path), Ndataqid);
		devdir(c, q, "data", 0, o, perm, dp);
		break;
	case 1:
		q.path = NETQID(NETID(c->qid.path), Nctlqid);
		devdir(c, q, "ctl", 0, o, perm, dp);
		break;
	case 2:
		q.path = NETQID(NETID(c->qid.path), Nstatqid);
		devdir(c, q, "stats", 0, eve, 0444, dp);
		break;
	case 3:
		q.path = NETQID(NETID(c->qid.path), Ntypeqid);
		devdir(c, q, "type", 0, eve, 0444, dp);
		break;
	case 4:
		q.path = NETQID(NETID(c->qid.path), Nifstatqid);
		devdir(c, q, "ifstats", 0, eve, 0444, dp);
		break;
	default:
		return -1;
	}
	return 1;
}

Walkqid*
netifwalk(Netif *nif, Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab *)nif, 0, netifgen);
}

Chan*
netifopen(Netif *nif, Chan *c, int omode)
{
	Proc *up = externup();
	int id;
	Netfile *f;

	id = 0;
	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Eperm);
	} else {
		switch(NETTYPE(c->qid.path)){
		case Ndataqid:
		case Nctlqid:
			id = NETID(c->qid.path);
			openfile(nif, id);
			break;
		case Ncloneqid:
			id = openfile(nif, -1);
			c->qid.path = NETQID(id, Nctlqid);
			break;
		default:
			if(omode != OREAD)
				error(Ebadarg);
		}
		switch(NETTYPE(c->qid.path)){
		case Ndataqid:
		case Nctlqid:
			f = nif->f[id];
			if(netown(f, up->user, omode&7) < 0){
				netifclose(nif, c);
				error(Eperm);
			}
			break;
		}
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	c->iounit = qiomaxatomic;
	return c;
}

int32_t
netifread(Netif *nif, Chan *c, void *a, int32_t n, int64_t off)
{
	Proc *up = externup();
	int i, j;
	Netfile *f;
	char *p;
	int32_t offset;

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, (Dirtab*)nif, 0, netifgen);

	offset = off;
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		f = nif->f[NETID(c->qid.path)];
		return qread(f->iq, a, n);
	case Nctlqid:
		return readnum(offset, a, n, NETID(c->qid.path), NUMSIZE);
	case Nstatqid:
		p = malloc(READSTR);
		if(p == nil)
			error(Enomem);
		j = snprint(p, READSTR, "in: %llud\n", nif->inpackets);
		j += snprint(p+j, READSTR-j, "link: %d\n", nif->link);
		j += snprint(p+j, READSTR-j, "out: %llud\n", nif->outpackets);
		j += snprint(p+j, READSTR-j, "crc errs: %llud\n", nif->crcs);
		j += snprint(p+j, READSTR-j, "overflows: %llud\n", nif->overflows);
		j += snprint(p+j, READSTR-j, "soft overflows: %llud\n", nif->soverflows);
		j += snprint(p+j, READSTR-j, "framing errs: %llud\n", nif->frames);
		j += snprint(p+j, READSTR-j, "buffer errs: %llud\n", nif->buffs);
		j += snprint(p+j, READSTR-j, "output errs: %llud\n", nif->oerrs);
		j += snprint(p+j, READSTR-j, "prom: %d\n", nif->prom);
		j += snprint(p+j, READSTR-j, "mbps: %d\n", nif->mbps);
		j += snprint(p+j, READSTR-j, "addr: ");
		for(i = 0; i < nif->alen; i++)
			j += snprint(p+j, READSTR-j, "%2.2ux", nif->addr[i]);
		snprint(p+j, READSTR-j, "\n");
		n = readstr(offset, a, n, p);
		free(p);
		return n;
	case Naddrqid:
		p = malloc(READSTR);
		j = 0;
		for(i = 0; i < nif->alen; i++)
			j += snprint(p+j, READSTR-j, "%2.2ux", nif->addr[i]);
		n = readstr(offset, a, n, p);
		free(p);
		return n;
	case Ntypeqid:
		f = nif->f[NETID(c->qid.path)];
		return readnum(offset, a, n, f->type, NUMSIZE);
	case Nifstatqid:
		return 0;
	case Nmtuqid:
		snprint(up->genbuf, sizeof up->genbuf, "%11.ud %11.ud %11.ud\n", nif->minmtu, nif->mtu, nif->maxmtu);
		return readstr(offset, a, n, up->genbuf);
	}
	error(Ebadarg);
	return -1;	/* not reached */
}

Block*
netifbread(Netif *nif, Chan *c, int32_t n, int64_t offset)
{
	if((c->qid.type & QTDIR) || NETTYPE(c->qid.path) != Ndataqid)
		return devbread(c, n, offset);

	return qbread(nif->f[NETID(c->qid.path)]->iq, n);
}

/*
 *  make sure this type isn't already in use on this device
 */
static int
typeinuse(Netif *nif, int type)
{
	Netfile *f, **fp, **efp;

	if(type <= 0)
		return 0;

	efp = &nif->f[nif->nfile];
	for(fp = nif->f; fp < efp; fp++){
		f = *fp;
		if(f == 0)
			continue;
		if(f->type == type)
			return 1;
	}
	return 0;
}

/*
 *  the devxxx.c that calls us handles writing data, it knows best
 */
int32_t
netifwrite(Netif *nif, Chan *c, void *a, int32_t n)
{
	Proc *up = externup();
	Netfile *f;
	int type, mtu;
	char *p, buf[64];
	uint8_t binaddr[Nmaxaddr];

	if(NETTYPE(c->qid.path) != Nctlqid)
		error(Eperm);

	if(n >= sizeof(buf))
		n = sizeof(buf)-1;
	memmove(buf, a, n);
	buf[n] = 0;

	if(waserror()){
		qunlock(nif);
		nexterror();
	}

	qlock(nif);
	f = nif->f[NETID(c->qid.path)];
	if((p = matchtoken(buf, "connect")) != 0){
		qclose(f->iq);
		type = atoi(p);
		if(typeinuse(nif, type))
			error(Einuse);
		f->type = type;
		if(f->type < 0)
			nif->all++;
		qreopen(f->iq);
	} else if(matchtoken(buf, "promiscuous")){
		if(f->prom == 0){
			if(nif->prom == 0 && nif->promiscuous != nil)
				nif->promiscuous(nif->arg, 1);
			f->prom = 1;
			nif->prom++;
		}
	} else if((p = matchtoken(buf, "scanbs")) != 0){
		/* scan for base stations */
		if(f->scan == 0){
			type = atoi(p);
			if(type < 5)
				type = 5;
			if(nif->scanbs != nil)
				nif->scanbs(nif->arg, type);
			f->scan = type;
			nif->_scan++;
		}
	} else if((p = matchtoken(buf, "mtu")) != 0){
		/* poor planning. */
		if(!iseve())
			error(Eperm);
		mtu = atoi(p);
		/* zero resets default. */
		if(mtu != 0)
		if(mtu < nif->minmtu || mtu > nif->maxmtu)
			error(Ebadarg);
		if(nif->hwmtu)
			nif->mtu = nif->hwmtu(nif->arg, mtu);
		else
			nif->mtu = mtu;
	} else if(matchtoken(buf, "l2bridge")){
		f->bridge |= 2;
	} else if(matchtoken(buf, "bridge")){
		f->bridge |= 1;
	} else if(matchtoken(buf, "headersonly")){
		f->headersonly = 1;
	} else if((p = matchtoken(buf, "addmulti")) != 0){
		if(parseaddr(binaddr, p, nif->alen) < 0)
			error("bad address");
		p = netmulti(nif, f, binaddr, 1);
		if(p)
			error(p);
	} else if((p = matchtoken(buf, "remmulti")) != 0){
		if(parseaddr(binaddr, p, nif->alen) < 0)
			error("bad address");
		p = netmulti(nif, f, binaddr, 0);
		if(p)
			error(p);
	} else
		n = -1;
	qunlock(nif);
	poperror();
	return n;
}

int32_t
netifwstat(Netif *nif, Chan *c, uint8_t *db, int32_t n)
{
	Proc *up = externup();
	Dir *dir;
	Netfile *f;
	int l;

	f = nif->f[NETID(c->qid.path)];
	if(f == 0)
		error(Enonexist);

	if(netown(f, up->user, OWRITE) < 0)
		error(Eperm);

	dir = smalloc(sizeof(Dir)+n);
	l = convM2D(db, n, &dir[0], (char*)&dir[1]);
	if(l == 0){
		free(dir);
		error(Eshortstat);
	}
	if(!emptystr(dir[0].uid))
		strncpy(f->owner, dir[0].uid, KNAMELEN);
	if(dir[0].mode != (uint32_t)~0UL)
		f->mode = dir[0].mode;
	free(dir);
	return l;
}

int32_t
netifstat(Netif *nif, Chan *c, uint8_t *db, int32_t n)
{
	return devstat(c, db, n, (Dirtab *)nif, 0, netifgen);
}

void
netifclose(Netif *nif, Chan *c)
{
	Netfile *f;
	int t;
	Netaddr *ap;

	if((c->flag & COPEN) == 0)
		return;

	t = NETTYPE(c->qid.path);
	if(t != Ndataqid && t != Nctlqid)
		return;

	f = nif->f[NETID(c->qid.path)];
	qlock(f);
	if(--(f->inuse) == 0){
		if(f->prom){
			qlock(nif);
			if(--(nif->prom) == 0 && nif->promiscuous != nil)
				nif->promiscuous(nif->arg, 0);
			qunlock(nif);
			f->prom = 0;
		}
		if(f->scan){
			qlock(nif);
			if(--(nif->_scan) == 0 && nif->scanbs != nil)
				nif->scanbs(nif->arg, 0);
			qunlock(nif);
			f->prom = 0;
			f->scan = 0;
		}
		if(f->nmaddr){
			qlock(nif);
			t = 0;
			for(ap = nif->maddr; ap; ap = ap->next){
				if(f->maddr[t/8] & (1<<(t%8)))
					netmulti(nif, f, ap->addr, 0);
			}
			qunlock(nif);
			f->nmaddr = 0;
		}
		if(f->type < 0){
			qlock(nif);
			--(nif->all);
			qunlock(nif);
		}
		f->owner[0] = 0;
		f->type = 0;
		f->bridge = 0;
		f->headersonly = 0;
		qclose(f->iq);
	}
	qunlock(f);
}

Lock netlock;

static int
netown(Netfile *p, char *o, int omode)
{
	static int access[] = { 0400, 0200, 0600, 0100 };
	int mode;
	int t;

	lock(&netlock);
	if(*p->owner){
		if(strncmp(o, p->owner, KNAMELEN) == 0)	/* User */
			mode = p->mode;
		else if(strncmp(o, eve, KNAMELEN) == 0)	/* Bootes is group */
			mode = p->mode<<3;
		else
			mode = p->mode<<6;		/* Other */

		t = access[omode&3];
		if((t & mode) == t){
			unlock(&netlock);
			return 0;
		} else {
			unlock(&netlock);
			return -1;
		}
	}
	strncpy(p->owner, o, KNAMELEN);
	p->mode = 0660;
	unlock(&netlock);
	return 0;
}

/*
 *  Increment the reference count of a network device.
 *  If id < 0, return an unused ether device.
 */
static int
openfile(Netif *nif, int id)
{
	Proc *up = externup();
	Netfile *f, **fp, **efp;

	if(id >= 0){
		f = nif->f[id];
		if(f == 0)
			error(Enodev);
		qlock(f);
		qreopen(f->iq);
		f->inuse++;
		qunlock(f);
		return id;
	}

	qlock(nif);
	if(waserror()){
		qunlock(nif);
		nexterror();
	}
	efp = &nif->f[nif->nfile];
	for(fp = nif->f; fp < efp; fp++){
		f = *fp;
		if(f == 0){
			f = malloc(sizeof(Netfile));
			if(f == 0)
				exhausted("memory");
			f->iq = qopen(nif->limit, Qmsg, 0, 0);
			if(f->iq == nil){
				free(f);
				exhausted("memory");
			}
			*fp = f;
			qlock(f);
		} else {
			qlock(f);
			if(f->inuse){
				qunlock(f);
				continue;
			}
		}
		f->inuse = 1;
		qreopen(f->iq);
		netown(f, up->user, 0);
		qunlock(f);
		qunlock(nif);
		poperror();
		return fp - nif->f;
	}
	error(Enodev);
	return -1;	/* not reached */
}

/*
 *  look for a token starting a string,
 *  return a pointer to first non-space char after it
 */
static char*
matchtoken(char *p, char *token)
{
	int n;

	n = strlen(token);
	if(strncmp(p, token, n))
		return 0;
	p += n;
	if(*p == 0)
		return p;
	if(*p != ' ' && *p != '\t' && *p != '\n')
		return 0;
	while(*p == ' ' || *p == '\t' || *p == '\n')
		p++;
	return p;
}

static uint32_t
hash(uint8_t *a, int len)
{
	uint32_t sum = 0;

	while(len-- > 0)
		sum = (sum << 1) + *a++;
	return sum%Nmhash;
}

int
activemulti(Netif *nif, uint8_t *addr, int alen)
{
	Netaddr *hp;

	for(hp = nif->mhash[hash(addr, alen)]; hp; hp = hp->hnext)
		if(memcmp(addr, hp->addr, alen) == 0){
			if(hp->ref)
				return 1;
			else
				break;
		}
	return 0;
}

static int
parseaddr(uint8_t *to, char *from, int alen)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	for(i = 0; i < alen; i++){
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

/*
 *  keep track of multicast addresses
 */
static char*
netmulti(Netif *nif, Netfile *f, uint8_t *addr, int add)
{
	Netaddr **l, *ap;
	int i;
	uint32_t h;

	if(nif->multicast == nil)
		return "interface does not support multicast";

	l = &nif->maddr;
	i = 0;
	for(ap = *l; ap; ap = *l){
		if(memcmp(addr, ap->addr, nif->alen) == 0)
			break;
		i++;
		l = &ap->next;
	}

	if(add){
		if(ap == 0){
			*l = ap = smalloc(sizeof(*ap));
			memmove(ap->addr, addr, nif->alen);
			ap->next = 0;
			ap->ref = 1;
			h = hash(addr, nif->alen);
			ap->hnext = nif->mhash[h];
			nif->mhash[h] = ap;
		} else {
			ap->ref++;
		}
		if(ap->ref == 1){
			nif->nmaddr++;
			nif->multicast(nif->arg, addr, 1);
		}
		if(i < 8*sizeof(f->maddr)){
			if((f->maddr[i/8] & (1<<(i%8))) == 0)
				f->nmaddr++;
			f->maddr[i/8] |= 1<<(i%8);
		}
	} else {
		if(ap == 0 || ap->ref == 0)
			return 0;
		ap->ref--;
		if(ap->ref == 0){
			nif->nmaddr--;
			nif->multicast(nif->arg, addr, 0);
		}
		if(i < 8*sizeof(f->maddr)){
			if((f->maddr[i/8] & (1<<(i%8))) != 0)
				f->nmaddr--;
			f->maddr[i/8] &= ~(1<<(i%8));
		}
	}
	return 0;
}
