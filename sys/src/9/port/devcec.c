/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Coraid ethernet console — serial replacement.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"
#include "../port/netif.h"

extern Dev cecdevtab;


enum {
	Namelen	= 128,
	Ncbuf 	= 8192,
	Ncmask 	= Ncbuf-1,
	Nconns	= 20,
	Size	= READSTR,
};

enum {
	Hdrsz	= 18,

	Tinita 	= 0,
	Tinitb,
	Tinitc,
	Tdata,
	Tack,
	Tdiscover,
	Toffer,
	Treset,
	Tlast,

	Cunused	= 0,
	Cinitb,
	Clogin,
	Copen,
};

static char *cstate[] = {
	"unused",
	"initb",
	"login",
	"open"
};

typedef struct {
	Chan	*dc;
	Chan	*cc;
	Dev	*d;
	uint8_t	ea[6];
	char	path[32];
} If;

typedef struct {
	uint8_t	dst[6];
	uint8_t	src[6];
	uint8_t	etype[2];
	uint8_t	type;
	uint8_t	conn;
	uint8_t	seq;
	uint8_t	len;
	uint8_t	data[0x100];
} Pkt;

typedef struct {
	QLock;
	Lock;
	unsigned char	ea[6];		/* along with cno, the key to the connection */
	unsigned char	cno;		/* connection number on remote host */
	unsigned char	stalled;		/* cectimer needs to kick it */
	int	state;		/* connection state */
	int	idle;		/* idle ticks */
	int	to;		/* ticks to timeout */
	int	retries;		/* remaining retries */
	Block	*bp;		/* unacked message */
	If	*ifc;		/* interface for this connection */
	unsigned char	sndseq;		/* sequence number of last sent message */
	unsigned char	rcvseq;		/* sequence number of last rcv'd message */
	char	cbuf[Ncbuf];	/* curcular buffer */
	int	r, w;		/* indexes into cbuf */
	int	pwi;		/* index into passwd; */
	char	passwd[32];	/* password typed by connection */
} Conn;

extern int parseether(uint8_t *, char *);
extern Chan *chandial(char *, char *, char *, Chan **);

enum {
	Qdir = 0,
	Qstat,
	Qctl,
	Qdbg,
	Qcfg,
	CMreset,
	CMsetshelf,
	CMsetname,
	CMtraceon,
	CMtraceoff,
	CMsetpasswd,
	CMcecon,
	CMcecoff,
	CMwrite,
};

static 	If 	ifs[4];
static 	char 	name[Namelen];
static	int	shelf = -1;
static 	Conn 	conn[Nconns];
static	int	tflag;
static	char	passwd[Namelen];
static	int	xmit;
static	int	rsnd;
static	Rendez	trendez;
static	int	tcond;
static	uint8_t	broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static 	Dirtab	cecdir[] = {
	".",		{ Qdir, 0, QTDIR },	0,	DMDIR | 0555,
	"cecstat",	{ Qstat},		1,	0444,
	"cecctl",	{ Qctl},		1,	0777,
//	"cecctl",	{ Qctl},		1,	0664,

	"cecdbg",	{ Qdbg }, 		1,	0444,
	"ceccfg",	{ Qcfg },		1,	0444,
};
static	Cmdtab	ceccmd[] = {
	CMsetname,	"name",	2,
	CMtraceon,	"traceon",	1,
	CMtraceoff,	"traceoff",	1,
	CMsetpasswd,	"password",	2,
	CMcecon,		"cecon",		2,
	CMcecoff,		"cecoff",		2,
	CMsetshelf,	"shelf",	2,
	CMwrite,	"write",	-1,
};

/*
 * Avoid outputting debugging to ourselves. Assumes a serial port.
 */
static int
cecprint(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof buf, fmt, arg)-buf;
	va_end(arg);
	uartputs(buf, n);
	return n;
}

static void
getaddr(char *path, uint8_t *ea)
{
	Proc *up = externup();
	char buf[6*2];
	int n;
	Chan *c;

	snprint(up->genbuf, sizeof up->genbuf, "%s/addr", path);
	c = namec(up->genbuf, Aopen, OREAD, 0);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	n = c->dev->read(c, buf, sizeof buf, 0);
	if(n != sizeof buf)
		error("getaddr");
	if(parseether(ea, buf) < 0)
		error("parseether failure");
	poperror();
	cclose(c);
}

static char *types[Tlast+1] = {
	"Tinita", "Tinitb", "Tinitc", "Tdata", "Tack",
	"Tdiscover", "Toffer", "Treset", "*GOK*",
};

static int
cbget(Conn *cp)
{
	int c;

	if(cp->r == cp->w)
		return -1;
	c = cp->cbuf[cp->r];
	cp->r = cp->r+1 & Ncmask;
	return c;
}

static void
cbput(Conn *cp, int c)
{
	if(cp->r == (cp->w+1 & Ncmask))
		return;
	cp->cbuf[cp->w] = c;
	cp->w = cp->w+1 & Ncmask;
}

static void
trace(Block *bp)
{
	uint type;
	Pkt *p;

	if(tflag == 0)
		return;
	p = (Pkt *)bp->rp;
	type = p->type;
	if(type > Treset)
		type = Treset+1;
	cecprint("%E > %E) seq %d, type %s, len %d, conn %d\n",
		p->src, p->dst, p->seq, types[type], p->len, p->conn);
}

static Block*
sethdr(If *ifc, uint8_t *ea, Pkt **npp, int len)
{
	Block *bp;
	Pkt *np;

	len += Hdrsz;
	if(len < 60)
		len = 60;
	bp = allocb(len);
	bp->wp = bp->rp+len;
	np = (Pkt *)bp->rp;
	memmove(np->dst, ea, 6);
	memmove(np->src, ifc->ea, 6);
	np->etype[0] = 0xbc;
	np->etype[1] = 0xbc;
	np->seq = 0;
	*npp = np;
	return bp;
}

static void
send(Conn *cp, Block *bp)
{
	Block *nbp;

	if(cp->bp != nil)
		panic("cecsend: cp->bp not nil\n");
	nbp = allocb(BLEN(bp));
	memmove(nbp->wp, bp->rp, BLEN(bp));
	nbp->wp += BLEN(bp);
	cp->bp = nbp;
	trace(bp);
	cp->ifc->d->bwrite(cp->ifc->dc, bp, 0);
	xmit++;
	cp->to = 4;
	cp->retries = 3;
	xmit++;
}

static void
senddata(Conn *cp, void *data, int len)
{
	Block *bp;
	Pkt *p;

	bp = sethdr(cp->ifc, cp->ea, &p, len);
	memmove(p->data, data, len);
	p->len = len;
	p->seq = ++cp->sndseq;
	p->conn = cp->cno;
	p->type = Tdata;
	send(cp, bp);
}

static void
resend(Conn *cp)
{
	Block *nbp;

	rsnd++;
	nbp = allocb(BLEN(cp->bp));
	memmove(nbp->wp, cp->bp->rp, BLEN(cp->bp));
	nbp->wp += BLEN(cp->bp);
	trace(nbp);
	cp->ifc->d->bwrite(cp->ifc->dc, nbp, 0);
	cp->to = 4;
}

#if 0
static void
reset(If *ifc, uint8_t conn)
{
	Block *bp;
	Pkt *p;

	bp = sethdr(ifc, ifc->ea, &p, 0);
	p->type = Treset;
	p->conn = conn;
	trace(bp);
	ifc->d->bwrite(ifc->dc, bp, 0);
}
#endif

static void
ack(Conn *cp)
{
	if(cp->bp)
		freeb(cp->bp);
	cp->bp = 0;
	cp->to = 0;
	cp->retries = 0;
}

static void
start(Conn *cp)
{
	char buf[250];
	int n, c;

	if(cp->bp)
		return;
	ilock(cp);
	for(n = 0; n < sizeof buf; n++){
		if((c = cbget(cp)) == -1)
			break;
		buf[n] = c;
	}
	iunlock(cp);
	if(n != 0)
		senddata(cp, buf, n);
}

static void
cecputs(char *str, int n)
{
	int i, c, wake;
	Conn *cp;

	wake = 0;
	for(cp = conn; cp < conn+Nconns; cp++){
		ilock(cp);
		if(cp->state == Copen){
			for (i = 0; i < n; i++){
				c = str[i];
				if(c == 0)
					continue;	/* BOTCH */
				if(c == '\n')
					cbput(cp, '\r');
				cbput(cp, c);
			}
			cp->stalled = 1;
			wake = 1;
		}
		iunlock(cp);
	}
	if(wake){
		tcond = 1;
		wakeup(&trendez);
	}
}

static void
conputs(Conn *c, char *s)
{
	for(; *s; s++)
		cbput(c, *s);
}

static int
icansleep(void* v)
{
	return tcond != 0;
}

static void
cectimer(void * v)
{
	Conn *cp;

	for(;;){
		tsleep(&trendez, icansleep, 0, 250);
		tcond = 0;
		for(cp = conn; cp < conn + Nconns; cp++){
			qlock(cp);
			if(cp->bp != nil){
				if(--cp->to <= 0){
					if(--cp->retries <= 0){
						freeb(cp->bp);
						cp->bp = 0;
//						cp->state = Cunused;
					}else
						resend(cp);
				}
			}else if(cp->stalled){
				cp->stalled = 0;
				start(cp);
			}
			qunlock(cp);
		}
	}
}

#if 0
static int
cecqlen(void* v)
{
	int n;
	Conn *c;

	n = 0;
	for(c = conn; c < conn+Nconns; c++){
		if(!canqlock(c))
			continue;
		if(c->bp)
			n += BLEN(c->bp);
		if(c->r > c->w)
			n += c->r-c->w;
		else
			n += c->w-c->r;
		qunlock(c);
	}
	if(n){
		tcond = 1;
		wakeup(&trendez);
	}
	return n;
}
#endif

static void
discover(If *ifc, Pkt *p)
{
	uint8_t *addr;
	Block *bp;
	Pkt *np;

	if(p)
		addr = p->src;
	else
		addr = broadcast;
	bp = sethdr(ifc, addr, &np, 0);
	np->type = Toffer;
	np->len = snprint((char *)np->data, sizeof np->data, "%d %s",
			  shelf, name);
	trace(bp);
	ifc->d->bwrite(ifc->dc, bp, 0);
}

static Conn*
connidx(int cno)
{
	Conn *c;

	for(c = conn; c < conn + Nconns; c++)
		if(cno == c->cno){
			qlock(c);
			return c;
		}
	return nil;
}

static Conn*
findconn(uint8_t *ea, uint8_t cno)
{
	Conn *cp, *ncp;

	ncp = nil;
	for(cp = conn; cp < conn + Nconns; cp++){
		if(ncp == nil && cp->state == Cunused)
			ncp = cp;
		if(memcmp(ea, cp->ea, 6) == 0 && cno == cp->cno){
			qlock(cp);
			return cp;
		}
	}
	if(ncp != nil)
		qlock(ncp);
	return ncp;
}

static void
checkpw(Conn *cp, char *str, int len)
{
	int i, c;

	if(passwd[0] == 0)
		return;
	for(i = 0; i < len; i++){
		c = str[i];
		if(c != '\n' && c != '\r'){
			if(cp->pwi < (sizeof cp->passwd)-1)
				cp->passwd[cp->pwi++] = c;
			cbput(cp, '#');
			cecprint("%c", c);
			continue;
		}
		/* is newline; check password */
		cp->passwd[cp->pwi] = 0;
		if(strcmp(cp->passwd, passwd) == 0){
			cp->state = Copen;
			cp->pwi = 0;
			print("\r\n%E logged in\r\n", cp->ea);
		}else{
			conputs(cp, "\r\nBad password\r\npassword: ");
			cp->pwi = 0;
		}
	}
	start(cp);
}

static void
incoming(Conn *cp, If *ifc, Pkt *p)
{
	int i;
	Block *bp;
	Pkt *np;

	/* ack it no matter what its sequence number */
	bp = sethdr(ifc, p->src, &np, 0);
	np->type = Tack;
	np->seq = p->seq;
	np->conn = cp->cno;
	np->len = 0;
	trace(bp);
	ifc->d->bwrite(ifc->dc, bp, 0);

	if(cp->state == Cunused){
		/*  connection */
		discover(ifc, p);
		return;
	}
	if(p->seq == cp->rcvseq)
		return;

	cp->rcvseq = p->seq;
	if(cp->state == Copen){
		for (i = 0; i < p->len; i++)
			kbdcr2nl(nil, (char)p->data[i]);
	}else if(cp->state == Clogin)
		checkpw(cp, (char *)p->data, p->len);
}

static void
inita(Conn *ncp, If *ifc, Pkt *p)
{
	Block *bp;
	Pkt *np;

	ncp->ifc = ifc;
	ncp->state = Cinitb;
	memmove(ncp->ea, p->src, 6);
	ncp->cno = p->conn;
	bp = sethdr(ifc, p->src, &np, 0);
	np->type = Tinitb;
	np->conn = ncp->cno;
	np->len = 0;
	send(ncp, bp);
}


static void
cecrdr(void *vp)
{
	Proc *up = externup();
	Block *bp;
	Conn *cp;
	If *ifc;
	Pkt *p;

	ifc = vp;
	if(waserror())
		goto exit;

	discover(ifc, 0);
	for(;;){
		bp = ifc->d->bread(ifc->dc, 1514, 0); // do we care about making the MTU non magic?
		if(bp == nil)
			nexterror();
		p = (Pkt *)bp->rp;
		if(p->etype[0] != 0xbc || p->etype[1] != 0xbc){
			freeb(bp);
			continue;
		}
		trace(bp);
		cp = findconn(p->src, p->conn);
		if(cp == nil){
			cecprint("cec: out of connection structures\n");
			freeb(bp);
			continue;
		}
		if (waserror()){
			freeb(bp);
			qunlock(cp);
			continue;
		}
		switch(p->type){
		case Tinita:
			if(cp->bp){
				cecprint("cec: reset with bp!? ask quanstro\n");
				freeb(cp->bp);
				cp->bp = 0;
			}
			inita(cp, ifc, p);
			break;
		case Tinitb:
			cecprint("cec: unexpected initb\n");
			break;
		case Tinitc:
			if(cp->state == Cinitb){
				ack(cp);
				if(cp->passwd[0]){
					cp->state = Clogin;
					conputs(cp, "password: ");
					start(cp);
				}else
					cp->state = Copen;
			}
			break;
		case Tdata:
			incoming(cp, ifc, p);
			break;
		case Tack:
			if(cp->state == Clogin || cp->state == Copen){
				ack(cp);
				start(cp);
			}
			break;
		case Tdiscover:
			discover(ifc, p);
			break;
		case Toffer:
			// cecprint("cec: unexpected offer\n"); from ourselves.
			break;
		case Treset:
			if(cp->bp)
				freeb(cp->bp);
			cp->bp = 0;
			cp->state = Cunused;
			break;
		default:
			cecprint("bad cec type: %d\n", p->type);
			break;
		}
		nexterror();
	}

exit:
	for(cp = conn; cp < conn+nelem(conn); cp++)
		if(cp->ifc == ifc){
			if(cp->bp)
				freeb(cp->bp);
			memset(cp, 0, sizeof *cp);
			break;
		}

	memset(ifc, 0, sizeof *ifc);
	pexit("cec exiting", 1);
}

static Chan *
cecattach(char *spec)
{
	Chan *c;
	static QLock q;
	static int inited;

	qlock(&q);
	if(inited == 0){
		kproc("cectimer", cectimer, nil);
		inited++;
	}
	qunlock(&q);
	c = devattach(L'©', spec);
	c->qid.path = Qdir;
	return c;
}

static Walkqid*
cecwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, cecdir, nelem(cecdir), devgen);
}

static int32_t
cecstat(Chan *c, uint8_t *dp, int32_t n)
{
	return devstat(c, dp, n, cecdir, nelem(cecdir), devgen);
}

static Chan*
cecopen(Chan *c, int omode)
{
	return devopen(c, omode, cecdir, nelem(cecdir), devgen);
}

static void
cecclose(Chan* c)
{
}

static int32_t
cecread(Chan *c, void *a, int32_t n, int64_t offset)
{
	char *p;
	int j;
	Conn *cp;
	If *ifc;

	switch((int)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, cecdir, nelem(cecdir), devgen);
	case Qstat:
		p = smalloc(Size);
		j = 0;
		for(cp = conn; cp < conn+Nconns; cp++)
			if(cp->state != Cunused)
			j += snprint(p+j, Size-j,
				"%E %3d %-6s %12d %d %d %.8lux\n",
				cp->ea, cp->cno, cstate[cp->state], cp->idle,
				cp->to, cp->retries, (uintptr_t)cp->bp);
		n = readstr(offset, a, n, p);
		free(p);
		return n;
	case Qdbg:
		cecprint("xmit %d, rsnd %d\n", xmit, rsnd);
		return 0;
	case Qcfg:
	case Qctl:
		p = smalloc(Size);
		j = 0;
		for(ifc = ifs; ifc < ifs+nelem(ifs); ifc++)
			if(ifc->d)
				j += snprint(p+j, Size-j, "%s\n", ifc->path);
		n = readstr(offset, a, n, p);
		free(p);
		return n;
	}
	error(Egreg);
	return 0;
}

/*
static void
cecfixup(void)
{
	char *p;
	int len;

	p = malloc(128*1024);
	snprint(p, 6, "write ");
	len = readfile("/dev/kmesg", p+6, 128*1024-6);
	writefile("#©/cecctl", p, len+6);
	free(p);
}
*/

static void
cecon(char *path)
{
	Proc *up = externup();
	char buf[64];
	uint8_t ea[6];
	Chan *dc, *cc;
	If *ifc, *nifc;

	nifc = nil;
	for(ifc = ifs; ifc < ifs+nelem(ifs); ifc++)
		if(ifc->d == nil)
			nifc = ifc;
		else if(strcmp(ifc->path, path) == 0)
			return;
	ifc = nifc;
	if(ifc == nil)
		error("out of interface structures");

	getaddr(path, ea);
	snprint(buf, sizeof buf, "%s!0xbcbc", path);
	dc = chandial(buf, nil, nil, &cc);
	if(dc == nil || cc == nil){
		if (cc)
			cclose(cc);
		if (dc)
			cclose(dc);
		snprint(up->genbuf, sizeof up->genbuf, "can't dial %s", buf);
		error(up->genbuf);
	}
	ifc->d = cc->dev;
	ifc->cc = cc;
	ifc->dc = dc;
	strncpy(ifc->path, path, sizeof ifc->path);
	memmove(ifc->ea, ea, 6);
	snprint(up->genbuf, sizeof up->genbuf, "cec:%s", path);
	kproc(up->genbuf, cecrdr, ifc);
}

static void
cecoff(char *path)
{
	int all, n;
	If *ifc, *e;

	all = strcmp(path, "*") == 0;
	n = 0;
	ifc = ifs;
	e = ifc+nelem(ifs);
	for(; ifc < e; ifc++)
		if(ifc->d && (all || strcmp(path, ifc->path) == 0)){
			cclose(ifc->cc);
			cclose(ifc->dc);
			memset(ifc, 0, sizeof *ifc);
			n++;
		}
	if(all + n == 0)
		error("cec not found");
}

static void
rst(Conn *c)
{
	if(c == nil)
		error("no such index");
	if(c->bp)
		freeb(c->bp);
	c->bp = 0;
	c->state = Cunused;
	qunlock(c);
}

static int32_t
cecwrite(Chan *c, void *a, int32_t n, int64_t mm)
{
	Proc *up = externup();
	Cmdbuf *cb;
	Cmdtab *cp;

	if(c->qid.path == Qctl){
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		cp = lookupcmd(cb, ceccmd, nelem(ceccmd));
		switch(cp->index){
		case CMsetname:
			strecpy(name, name+sizeof name-1, cb->f[1]);
			break;
		case CMtraceon:
			tflag = 1;
			break;
		case CMtraceoff:
			tflag = 0;
			break;
		case CMsetpasswd:
			strcpy(passwd, cb->f[1]);
			break;
		case CMcecon:
			cecon(cb->f[1]);
			break;
		case CMcecoff:
			cecoff(cb->f[1]);
			break;
		case CMsetshelf:
			shelf = atoi(cb->f[1]);
			break;
		case CMwrite:
			cecputs((char*)a+6,n-6);
			break;
		case CMreset:
			rst(connidx(atoi(cb->f[1])));
			break;
		default:
			cmderror(cb, "bad control message");
			break;
		}
		free(cb);
		poperror();
		return n;
	}
	error(Egreg);
	return 0;
}

static void
cecinit(void)
{
	addconsdev(nil, cecputs, -1, 0);
}

Dev cecdevtab = {
	.dc = L'©',
	.name = "cec",

	.reset = devreset,
	.init = cecinit,
	.shutdown = devshutdown,
	.attach = cecattach,
	.walk = cecwalk,
	.stat = cecstat,
	.open = cecopen,
	.create = devcreate,
	.close = cecclose,
	.read = cecread,
	.bread = devbread,
	.write = cecwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
	.power = devpower,
	.config = devconfig,
};
