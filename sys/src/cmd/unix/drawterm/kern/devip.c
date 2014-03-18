#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "ip.h"

#include "devip.h"

void	csclose(Chan*);
long	csread(Chan*, void*, long, vlong);
long	cswrite(Chan*, void*, long, vlong);

void osipinit(void);

enum
{
	Qtopdir		= 1,	/* top level directory */
	Qcs,
	Qprotodir,		/* directory for a protocol */
	Qclonus,
	Qconvdir,		/* directory for a conversation */
	Qdata,
	Qctl,
	Qstatus,
	Qremote,
	Qlocal,
	Qlisten,

	MAXPROTO	= 4
};
#define TYPE(x) 	((int)((x).path & 0xf))
#define CONV(x) 	((int)(((x).path >> 4)&0xfff))
#define PROTO(x) 	((int)(((x).path >> 16)&0xff))
#define QID(p, c, y) 	(((p)<<16) | ((c)<<4) | (y))
#define ipzero(x)	memset(x, 0, IPaddrlen)

typedef struct Proto	Proto;
typedef struct Conv	Conv;
struct Conv
{
	int	x;
	Ref	r;
	int	sfd;
	int	perm;
	char	owner[KNAMELEN];
	char*	state;
	uchar	laddr[IPaddrlen];
	ushort	lport;
	uchar	raddr[IPaddrlen];
	ushort	rport;
	int	restricted;
	char	cerr[KNAMELEN];
	Proto*	p;
};

struct Proto
{
	Lock	l;
	int	x;
	int	stype;
	char	name[KNAMELEN];
	int	nc;
	int	maxconv;
	Conv**	conv;
	Qid	qid;
};

static	int	np;
static	Proto	proto[MAXPROTO];

static	Conv*	protoclone(Proto*, char*, int);
static	void	setladdr(Conv*);

int
ipgen(Chan *c, char *nname, Dirtab *d, int nd, int s, Dir *dp)
{
	Qid q;
	Conv *cv;
	char *p;

	USED(nname);
	q.vers = 0;
	q.type = 0;
	switch(TYPE(c->qid)) {
	case Qtopdir:
		if(s >= 1+np)
			return -1;

		if(s == 0){
			q.path = QID(s, 0, Qcs);
			devdir(c, q, "cs", 0, "network", 0666, dp);
		}else{
			s--;
			q.path = QID(s, 0, Qprotodir);
			q.type = QTDIR;
			devdir(c, q, proto[s].name, 0, "network", DMDIR|0555, dp);
		}
		return 1;
	case Qprotodir:
		if(s < proto[PROTO(c->qid)].nc) {
			cv = proto[PROTO(c->qid)].conv[s];
			sprint(up->genbuf, "%d", s);
			q.path = QID(PROTO(c->qid), s, Qconvdir);
			q.type = QTDIR;
			devdir(c, q, up->genbuf, 0, cv->owner, DMDIR|0555, dp);
			return 1;
		}
		s -= proto[PROTO(c->qid)].nc;
		switch(s) {
		default:
			return -1;
		case 0:
			p = "clone";
			q.path = QID(PROTO(c->qid), 0, Qclonus);
			break;
		}
		devdir(c, q, p, 0, "network", 0555, dp);
		return 1;
	case Qconvdir:
		cv = proto[PROTO(c->qid)].conv[CONV(c->qid)];
		switch(s) {
		default:
			return -1;
		case 0:
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qdata);
			devdir(c, q, "data", 0, cv->owner, cv->perm, dp);
			return 1;
		case 1:
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qctl);
			devdir(c, q, "ctl", 0, cv->owner, cv->perm, dp);
			return 1;
		case 2:
			p = "status";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qstatus);
			break;
		case 3:
			p = "remote";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qremote);
			break;
		case 4:
			p = "local";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qlocal);
			break;
		case 5:
			p = "listen";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qlisten);
			break;
		}
		devdir(c, q, p, 0, cv->owner, 0444, dp);
		return 1;
	}
	return -1;
}

static void
newproto(char *name, int type, int maxconv)
{
	int l;
	Proto *p;

	if(np >= MAXPROTO) {
		print("no %s: increase MAXPROTO", name);
		return;
	}

	p = &proto[np];
	strcpy(p->name, name);
	p->stype = type;
	p->qid.path = QID(np, 0, Qprotodir);
	p->qid.type = QTDIR;
	p->x = np++;
	p->maxconv = maxconv;
	l = sizeof(Conv*)*(p->maxconv+1);
	p->conv = mallocz(l, 1);
	if(p->conv == 0)
		panic("no memory");
}

void
ipinit(void)
{
	osipinit();

	newproto("udp", S_UDP, 10);
	newproto("tcp", S_TCP, 30);

	fmtinstall('I', eipfmt);
	fmtinstall('E', eipfmt);
	
}

Chan *
ipattach(char *spec)
{
	Chan *c;

	c = devattach('I', spec);
	c->qid.path = QID(0, 0, Qtopdir);
	c->qid.type = QTDIR;
	c->qid.vers = 0;
	return c;
}

static Walkqid*
ipwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, ipgen);
}

int
ipstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, 0, 0, ipgen);
}

Chan *
ipopen(Chan *c, int omode)
{
	Proto *p;
	uchar raddr[IPaddrlen];
	ushort rport;
	int perm, sfd;
	Conv *cv, *lcv;

	omode &= 3;
	perm = 0;
	switch(omode) {
	case OREAD:
		perm = 4;
		break;
	case OWRITE:
		perm = 2;
		break;
	case ORDWR:
		perm = 6;
		break;
	}

	switch(TYPE(c->qid)) {
	default:
		break;
	case Qtopdir:
	case Qprotodir:
	case Qconvdir:
	case Qstatus:
	case Qremote:
	case Qlocal:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qclonus:
		p = &proto[PROTO(c->qid)];
		cv = protoclone(p, up->user, -1);
		if(cv == 0)
			error(Enodev);
		c->qid.path = QID(p->x, cv->x, Qctl);
		c->qid.vers = 0;
		break;
	case Qdata:
	case Qctl:
		p = &proto[PROTO(c->qid)];
		lock(&p->l);
		cv = p->conv[CONV(c->qid)];
		lock(&cv->r.lk);
		if((perm & (cv->perm>>6)) != perm) {
			if(strcmp(up->user, cv->owner) != 0 ||
		 	  (perm & cv->perm) != perm) {
				unlock(&cv->r.lk);
				unlock(&p->l);
				error(Eperm);
			}
		}
		cv->r.ref++;
		if(cv->r.ref == 1) {
			memmove(cv->owner, up->user, KNAMELEN);
			cv->perm = 0660;
		}
		unlock(&cv->r.lk);
		unlock(&p->l);
		break;
	case Qlisten:
		p = &proto[PROTO(c->qid)];
		lcv = p->conv[CONV(c->qid)];
		sfd = so_accept(lcv->sfd, raddr, &rport);
		cv = protoclone(p, up->user, sfd);
		if(cv == 0) {
			close(sfd);
			error(Enodev);
		}
		ipmove(cv->raddr, raddr);
		cv->rport = rport;
		setladdr(cv);
		cv->state = "Established";
		c->qid.path = QID(p->x, cv->x, Qctl);
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
ipclose(Chan *c)
{
	Conv *cc;

	switch(TYPE(c->qid)) {
	case Qcs:
		csclose(c);
		break;
	case Qdata:
	case Qctl:
		if((c->flag & COPEN) == 0)
			break;
		cc = proto[PROTO(c->qid)].conv[CONV(c->qid)];
		if(decref(&cc->r) != 0)
			break;
		strcpy(cc->owner, "network");
		cc->perm = 0666;
		cc->state = "Closed";
		ipzero(cc->laddr);
		ipzero(cc->raddr);
		cc->lport = 0;
		cc->rport = 0;
		close(cc->sfd);
		break;
	}
}

long
ipread(Chan *ch, void *a, long n, vlong offset)
{
	int r;
	Conv *c;
	Proto *x;
	uchar ip[IPaddrlen];
	char buf[128], *p;

/*print("ipread %s %lux\n", c2name(ch), (long)ch->qid.path);*/
	p = a;
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qcs:
		return csread(ch, a, n, offset);
	case Qprotodir:
	case Qtopdir:
	case Qconvdir:
		return devdirread(ch, a, n, 0, 0, ipgen);
	case Qctl:
		sprint(buf, "%d", CONV(ch->qid));
		return readstr(offset, p, n, buf);
	case Qremote:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		ipmove(ip, c->raddr);
		sprint(buf, "%I!%d\n", ip, c->rport);
		return readstr(offset, p, n, buf);
	case Qlocal:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		ipmove(ip, c->laddr);
		sprint(buf, "%I!%d\n", ip, c->lport);
		return readstr(offset, p, n, buf);
	case Qstatus:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		sprint(buf, "%s/%d %d %s \n",
			c->p->name, c->x, c->r.ref, c->state);
		return readstr(offset, p, n, buf);
	case Qdata:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		r = so_recv(c->sfd, a, n, 0);
		if(r < 0){
			oserrstr();
			nexterror();
		}
		return r;
	}
}

static void
setladdr(Conv *c)
{
	so_getsockname(c->sfd, c->laddr, &c->lport);
}

static void
setlport(Conv *c)
{
	if(c->restricted == 0 && c->lport == 0)
		return;

	if(c->sfd == -1)
		c->sfd = so_socket(c->p->stype, c->laddr);

	so_bind(c->sfd, c->restricted, c->lport, c->laddr);
}

static void
setladdrport(Conv *c, char *str)
{
	char *p;
	uchar addr[IPaddrlen];

	p = strchr(str, '!');
	if(p == 0) {
		p = str;
		ipzero(c->laddr);
	}
	else {
		*p++ = 0;
		parseip(addr, str);
		ipmove(c->laddr, addr);
	}
	if(*p == '*')
		c->lport = 0;
	else
		c->lport = atoi(p);

	setlport(c);
}

static char*
setraddrport(Conv *c, char *str)
{
	char *p;
	uchar addr[IPaddrlen];

	p = strchr(str, '!');
	if(p == 0)
		return "malformed address";
	*p++ = 0;
	parseip(addr, str);
	ipmove(c->raddr, addr);
	c->rport = atoi(p);
	p = strchr(p, '!');
	if(p) {
		if(strcmp(p, "!r") == 0)
			c->restricted = 1;
	}
	return 0;
}

long
ipwrite(Chan *ch, void *a, long n, vlong offset)
{
	Conv *c;
	Proto *x;
	int r, nf;
	char *p, *fields[3], buf[128];

	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qcs:
		return cswrite(ch, a, n, offset);
	case Qctl:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		if(n > sizeof(buf)-1)
			n = sizeof(buf)-1;
		memmove(buf, a, n);
		buf[n] = '\0';

		nf = tokenize(buf, fields, 3);
		if(strcmp(fields[0], "connect") == 0){
			switch(nf) {
			default:
				error("bad args to connect");
			case 2:
				p = setraddrport(c, fields[1]);
				if(p != 0)
					error(p);
				break;
			case 3:
				p = setraddrport(c, fields[1]);
				if(p != 0)
					error(p);
				c->lport = atoi(fields[2]);
				setlport(c);
				break;
			}
			if(c->sfd == -1)
				c->sfd = so_socket(c->p->stype, c->raddr);
			so_connect(c->sfd, c->raddr, c->rport);
			setladdr(c);
			c->state = "Established";
			return n;
		}
		if(strcmp(fields[0], "announce") == 0) {
			switch(nf){
			default:
				error("bad args to announce");
			case 2:
				setladdrport(c, fields[1]);
				break;
			}
			so_listen(c->sfd);
			c->state = "Announced";
			return n;
		}
		if(strcmp(fields[0], "bind") == 0){
			switch(nf){
			default:
				error("bad args to bind");
			case 2:
				c->lport = atoi(fields[1]);
				break;
			}
			setlport(c);
			return n;
		}
		error("bad control message");
	case Qdata:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		r = so_send(c->sfd, a, n, 0);
		if(r < 0){
			oserrstr();
			nexterror();
		}
		return r;
	}
	return n;
}

static Conv*
protoclone(Proto *p, char *user, int nfd)
{
	Conv *c, **pp, **ep;

	c = 0;
	lock(&p->l);
	if(waserror()) {
		unlock(&p->l);
		nexterror();
	}
	ep = &p->conv[p->maxconv];
	for(pp = p->conv; pp < ep; pp++) {
		c = *pp;
		if(c == 0) {
			c = mallocz(sizeof(Conv), 1);
			if(c == 0)
				error(Enomem);
			lock(&c->r.lk);
			c->r.ref = 1;
			c->p = p;
			c->x = pp - p->conv;
			p->nc++;
			*pp = c;
			break;
		}
		lock(&c->r.lk);
		if(c->r.ref == 0) {
			c->r.ref++;
			break;
		}
		unlock(&c->r.lk);
	}
	if(pp >= ep) {
		unlock(&p->l);
		poperror();
		return 0;
	}

	strcpy(c->owner, user);
	c->perm = 0660;
	c->state = "Closed";
	c->restricted = 0;
	ipzero(c->laddr);
	ipzero(c->raddr);
	c->lport = 0;
	c->rport = 0;
	c->sfd = nfd;

	unlock(&c->r.lk);
	unlock(&p->l);
	poperror();
	return c;
}

void
csclose(Chan *c)
{
	free(c->aux);
}

long
csread(Chan *c, void *a, long n, vlong offset)
{
	if(c->aux == nil)
		return 0;
	return readstr(offset, a, n, c->aux);
}

static struct
{
	char *name;
	uint num;
} tab[] = {
	"cs", 1,
	"echo", 7,
	"discard", 9,
	"systat", 11,
	"daytime", 13,
	"netstat", 15,
	"chargen", 19,
	"ftp-data", 20,
	"ftp", 21,
	"ssh", 22,
	"telnet", 23,
	"smtp", 25,
	"time", 37,
	"whois", 43,
	"dns", 53,
	"domain", 53,
	"uucp", 64,
	"gopher", 70,
	"rje", 77,
	"finger", 79,
	"http", 80,
	"link", 87,
	"supdup", 95,
	"hostnames", 101,
	"iso-tsap", 102,
	"x400", 103,
	"x400-snd", 104,
	"csnet-ns", 105,
	"pop-2", 109,
	"pop3", 110,
	"portmap", 111,
	"uucp-path", 117,
	"nntp", 119,
	"netbios", 139,
	"imap4", 143,
	"NeWS", 144,
	"print-srv", 170,
	"z39.50", 210,
	"fsb", 400,
	"sysmon", 401,
	"proxy", 402,
	"proxyd", 404,
	"https", 443,
	"cifs", 445,
	"ssmtp", 465,
	"rexec", 512,
	"login", 513,
	"shell", 514,
	"printer", 515,
	"courier", 530,
	"cscan", 531,
	"uucp", 540,
	"snntp", 563,
	"9fs", 564,
	"whoami", 565,
	"guard", 566,
	"ticket", 567,
	"dlsftp", 666,
	"fmclient", 729,
	"imaps", 993,
	"pop3s", 995,
	"ingreslock", 1524,
	"pptp", 1723,
	"nfs", 2049,
	"webster", 2627,
	"weather", 3000,
	"secstore", 5356,
	"Xdisplay", 6000,
	"styx", 6666,
	"mpeg", 6667,
	"rstyx", 6668,
	"infdb", 6669,
	"infsigner", 6671,
	"infcsigner", 6672,
	"inflogin", 6673,
	"bandt", 7330,
	"face", 32000,
	"dhashgate", 11978,
	"exportfs", 17007,
	"rexexec", 17009,
	"ncpu", 17010,
	"cpu", 17013,
	"glenglenda1", 17020,
	"glenglenda2", 17021,
	"glenglenda3", 17022,
	"glenglenda4", 17023,
	"glenglenda5", 17024,
	"glenglenda6", 17025,
	"glenglenda7", 17026,
	"glenglenda8", 17027,
	"glenglenda9", 17028,
	"glenglenda10", 17029,
	"flyboy", 17032,
	"dlsftp", 17033,
	"venti", 17034,
	"wiki", 17035,
	"vica", 17036,
	0
};

static int
lookupport(char *s)
{
	int i;
	char buf[10], *p;

	i = strtol(s, &p, 0);
	if(*s && *p == 0)
		return i;

	i = so_getservbyname(s, "tcp", buf);
	if(i != -1)
		return atoi(buf);
	for(i=0; tab[i].name; i++)
		if(strcmp(s, tab[i].name) == 0)
			return tab[i].num;
	return 0;
}

static int
lookuphost(char *s, uchar *to)
{
	ipzero(to);
	if(parseip(to, s) != -1)
		return 0;
	if((s = hostlookup(s)) == nil)
		return -1;
	parseip(to, s);
	free(s);
	return 0;
}

long
cswrite(Chan *c, void *a, long n, vlong offset)
{
	char *f[4];
	char *s, *ns;
	uchar ip[IPaddrlen];
	int nf, port;

	s = malloc(n+1);
	if(s == nil)
		error(Enomem);
	if(waserror()){
		free(s);
		nexterror();
	}
	memmove(s, a, n);
	s[n] = 0;
	nf = getfields(s, f, nelem(f), 0, "!");
	if(nf != 3)
		error("can't translate");

	port = lookupport(f[2]);
	if(port <= 0)
		error("no translation for port found");

	if(lookuphost(f[1], ip) < 0)
		error("no translation for host found");

	ns = smprint("/net/%s/clone %I!%d", f[0], ip, port);
	if(ns == nil)
		error(Enomem);
	free(c->aux);
	c->aux = ns;
	poperror();
	free(s);
	return n;
}

Dev ipdevtab = 
{
	'I',
	"ip",

	devreset,
	ipinit,
	devshutdown,
	ipattach,
	ipwalk,
	ipstat,
	ipopen,
	devcreate,
	ipclose,
	ipread,
	devbread,
	ipwrite,
	devbwrite,
	devremove,
	devwstat,
};

