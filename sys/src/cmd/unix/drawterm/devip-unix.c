#include <stdio.h>	/* for sys_errlist, of course */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "lib9.h"
#include "sys.h"
#include "error.h"

#undef listen
#undef accept
#undef bind

enum
{
	S_TCP,
	S_UDP
};

int		so_socket(int type);
void		so_connect(int, unsigned long, unsigned short);
void		so_getsockname(int, unsigned long*, unsigned short*);
void		so_bind(int, int, unsigned short);
void		so_listen(int);
int		so_accept(int, unsigned long*, unsigned short*);
int		so_getservbyname(char*, char*, char*);
int		so_gethostbyname(char*, char**, int);

static void		hnputl(void *p, unsigned long v);
static void		hnputs(void *p, unsigned short v);
static unsigned long	nhgetl(void *p);
static unsigned short	nhgets(void *p);
static unsigned long	parseip(char *to, char *from);

enum
{
	Qtopdir		= 1,	/* top level directory */
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
#define TYPE(x) 	((x).path & 0xf)
#define CONV(x) 	(((x).path >> 4)&0xfff)
#define PROTO(x) 	(((x).path >> 16)&0xff)
#define QID(p, c, y) 	(((p)<<16) | ((c)<<4) | (y))

typedef struct Proto	Proto;
typedef struct Conv	Conv;
struct Conv
{
	int	x;
	Ref	r;
	int	sfd;
	int	perm;
	char	owner[NAMELEN];
	char*	state;
	ulong	laddr;
	ushort	lport;
	ulong	raddr;
	ushort	rport;
	int	restricted;
	char	cerr[NAMELEN];
	Proto*	p;
};

struct Proto
{
	Lock	l;
	int	x;
	int	stype;
	char	name[NAMELEN];
	int	nc;
	int	maxconv;
	Conv**	conv;
	Qid	qid;
};

static	int	np;
static	Proto	proto[MAXPROTO];
static	int	eipconv(va_list*, Fconv*);
static	Conv*	protoclone(Proto*, char*, int);
static	void	setladdr(Conv*);

int
ipgen(Chan *c, Dirtab *d, int nd, int s, Dir *dp)
{
	Qid q;
	Conv *cv;
	char name[16], *p;

	q.vers = 0;
	switch(TYPE(c->qid)) {
	case Qtopdir:
		if(s >= np)
			return -1;

		q.path = QID(s, 0, Qprotodir)|CHDIR;
		devdir(c, q, proto[s].name, 0, "network", CHDIR|0555, dp);
		return 1;
	case Qprotodir:
		if(s < proto[PROTO(c->qid)].nc) {
			cv = proto[PROTO(c->qid)].conv[s];
			sprint(name, "%d", s);
			q.path = QID(PROTO(c->qid), s, Qconvdir)|CHDIR;
			devdir(c, q, name, 0, cv->owner, CHDIR|0555, dp);
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
	p->qid.path = CHDIR|QID(np, 0, Qprotodir);
	p->x = np++;
	p->maxconv = maxconv;
	l = sizeof(Conv*)*(p->maxconv+1);
	p->conv = mallocz(l);
	if(p->conv == 0)
		panic("no memory");
}

void
ipinit(void)
{
	gethostname(sysname, sizeof(sysname));
	newproto("udp", S_UDP, 10);
	newproto("tcp", S_TCP, 30);

	fmtinstall('i', eipconv);
	fmtinstall('I', eipconv);
	fmtinstall('E', eipconv);
	
}

Chan *
ipattach(void *spec)
{
	Chan *c;

	c = devattach('I', spec);
	c->qid.path = QID(0, 0, Qtopdir)|CHDIR;
	c->qid.vers = 0;
	return c;
}

Chan *
ipclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
ipwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, ipgen);
}

void
ipstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, ipgen);
}

Chan *
ipopen(Chan *c, int omode)
{
	Proto *p;
	ulong raddr;
	ushort rport;
	int perm, sfd;
	Conv *cv, *lcv;

	omode &= 3;
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
		lock(&cv->r.l);
		if((perm & (cv->perm>>6)) != perm) {
			if(strcmp(up->user, cv->owner) != 0 ||
		 	  (perm & cv->perm) != perm) {
				unlock(&cv->r.l);
				unlock(&p->l);
				error(Eperm);
			}
		}
		cv->r.ref++;
		if(cv->r.ref == 1) {
			memmove(cv->owner, up->user, NAMELEN);
			cv->perm = 0660;
		}
		unlock(&cv->r.l);
		unlock(&p->l);
		break;
	case Qlisten:
		p = &proto[PROTO(c->qid)];
		lcv = p->conv[CONV(c->qid)];
		sfd = so_accept(lcv->sfd, &raddr, &rport);
		cv = protoclone(p, up->user, sfd);
		if(cv == 0) {
			close(sfd);
			error(Enodev);
		}
		cv->raddr = raddr;
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
ipcreate(Chan *c, char *name, int mode, ulong perm)
{
	error(Eperm);
}

void
ipremove(Chan *c)
{
	error(Eperm);
}

void
ipwstat(Chan *c, char *buf)
{
	error(Eperm);
}

void
ipclose(Chan *c)
{
	Conv *cc;

	switch(TYPE(c->qid)) {
	case Qdata:
	case Qctl:
		if((c->flag & COPEN) == 0)
			break;
		cc = proto[PROTO(c->qid)].conv[CONV(c->qid)];
		if(refdec(&cc->r) != 0)
			break;
		strcpy(cc->owner, "network");
		cc->perm = 0666;
		cc->state = "Closed";
		cc->laddr = 0;
		cc->raddr = 0;
		cc->lport = 0;
		cc->rport = 0;
		close(cc->sfd);
		break;
	}
}

long
ipread(Chan *ch, void *a, long n, ulong offset)
{
	int r;
	Conv *c;
	Proto *x;
	uchar ip[4];
	char buf[128], *p;

	p = a;
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qprotodir:
	case Qtopdir:
	case Qconvdir:
		return devdirread(ch, a, n, 0, 0, ipgen);
	case Qctl:
		sprint(buf, "%d", CONV(ch->qid));
		return readstr(offset, p, n, buf);
	case Qremote:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		hnputl(ip, c->raddr);
		sprint(buf, "%I!%d\n", ip, c->rport);
		return readstr(offset, p, n, buf);
	case Qlocal:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		hnputl(ip, c->laddr);
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
		r = recv(c->sfd, a, n, 0);
		if(r < 0)
			error(strerror(errno));
		return r;
	}
}

static void
setladdr(Conv *c)
{
	so_getsockname(c->sfd, &c->laddr, &c->lport);
}

static void
setlport(Conv *c)
{
	if(c->restricted == 0 && c->lport == 0)
		return;

	so_bind(c->sfd, c->restricted, c->lport);
}

static void
setladdrport(Conv *c, char *str)
{
	char *p, addr[4];

	p = strchr(str, '!');
	if(p == 0) {
		p = str;
		c->laddr = 0;
	}
	else {
		*p++ = 0;
		parseip(addr, str);
		c->laddr = nhgetl((uchar*)addr);
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
	char *p, addr[4];

	p = strchr(str, '!');
	if(p == 0)
		return "malformed address";
	*p++ = 0;
	parseip(addr, str);
	c->raddr = nhgetl((uchar*)addr);
	c->rport = atoi(p);
	p = strchr(p, '!');
	if(p) {
		if(strcmp(p, "!r") == 0)
			c->restricted = 1;
	}
	return 0;
}

long
ipwrite(Chan *ch, void *a, long n, ulong offset)
{
	Conv *c;
	Proto *x;
	int r, nf;
	char *p, *fields[3], buf[128];

	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qctl:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		if(n > sizeof(buf)-1)
			n = sizeof(buf)-1;
		memmove(buf, a, n);
		buf[n] = '\0';

		nf = getfields(buf, fields, 3, 1, " ");
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
		r = send(c->sfd, a, n, 0);
		if(r < 0)
			error(strerror(errno));
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
			c = mallocz(sizeof(Conv));
			if(c == 0)
				error(Enomem);
			lock(&c->r.l);
			c->r.ref = 1;
			c->p = p;
			c->x = pp - p->conv;
			p->nc++;
			*pp = c;
			break;
		}
		lock(&c->r.l);
		if(c->r.ref == 0) {
			c->r.ref++;
			break;
		}
		unlock(&c->r.l);
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
	c->laddr = 0;
	c->raddr = 0;
	c->lport = 0;
	c->rport = 0;
	c->sfd = nfd;
	if(nfd == -1)
		c->sfd = so_socket(p->stype);

	unlock(&c->r.l);
	unlock(&p->l);
	poperror();
	return c;
}

static int
eipconv(va_list *v, Fconv *f)
{
	static char buf[64];
	static char *efmt = "%.2lux%.2lux%.2lux%.2lux%.2lux%.2lux";
	static char *ifmt = "%d.%d.%d.%d";
	uchar *p, ip[4];

	switch(f->chr) {
	case 'E':		/* Ethernet address */
		p = va_arg(*v, uchar*);
		sprint(buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	case 'I':		/* Ip address */
		p = va_arg(*v, uchar*);
		sprint(buf, ifmt, p[0], p[1], p[2], p[3]);
		break;
	case 'i':
		hnputl(ip, va_arg(*v, ulong));
		sprint(buf, ifmt, ip[0], ip[1], ip[2], ip[3]);
		break;
	default:
		strcpy(buf, "(eipconv)");
	}
	strconv(buf, f);
	return 0;
}
int
so_socket(int type)
{
	int fd;

	switch(type) {
	default:
		error("bad protocol type");
	case S_TCP:
		type = SOCK_STREAM;
		break;
	case S_UDP:
		type = SOCK_DGRAM;
		break;
	}

	fd = socket(AF_INET, type, 0);
	if(fd < 0)
		error(strerror(errno));
	return fd;
}


void
so_connect(int fd, unsigned long raddr, unsigned short rport)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, rport);
	hnputl(&sin.sin_addr.s_addr, raddr);

	if(connect(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		error(strerror(errno));
}

void
so_getsockname(int fd, unsigned long *laddr, unsigned short *lport)
{
	int len;
	struct sockaddr_in sin;

	len = sizeof(sin);
	if(getsockname(fd, (struct sockaddr*)&sin, &len) < 0)
		error(strerror(errno));

	if(sin.sin_family != AF_INET || len != sizeof(sin))
		error("not AF_INET");

	*laddr = nhgetl(&sin.sin_addr.s_addr);
	*lport = nhgets(&sin.sin_port);
}

void
so_listen(int fd)
{
	if(listen(fd, 5) < 0)
		error(strerror(errno));
}

int
so_accept(int fd, unsigned long *raddr, unsigned short *rport)
{
	int nfd, len;
	struct sockaddr_in sin;

	len = sizeof(sin);
	nfd = accept(fd, (struct sockaddr*)&sin, &len);
	if(nfd < 0)
		error(strerror(errno));

	if(sin.sin_family != AF_INET || len != sizeof(sin))
		error("not AF_INET");

	*raddr = nhgetl(&sin.sin_addr.s_addr);
	*rport = nhgets(&sin.sin_port);
	return nfd;
}

void
so_bind(int fd, int su, unsigned short port)
{
	int i, one;
	struct sockaddr_in sin;

	one = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0)
		print("setsockopt: %s", strerror(errno));

	if(su) {
		for(i = 600; i < 1024; i++) {
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_port = i;

			if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) >= 0)	
				return;
		}
		error(strerror(errno));
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, port);

	if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		error(strerror(errno));
}

int
so_gethostbyname(char *host, char**hostv, int n)
{
	int i;
	char buf[32];
	unsigned char *p;
	struct hostent *hp;

	hp = gethostbyname(host);
	if(hp == 0)
		return 0;

	for(i = 0; hp->h_addr_list[i] && i < n; i++) {
		p = (unsigned char*)hp->h_addr_list[i];
		sprint(buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
		hostv[i] = strdup(buf);
		if(hostv[i] == 0)
			break;
	}
	return i;
}

char*
hostlookup(char *host)
{
	char buf[100];
	uchar *p;
	struct hostent *he;

	he = gethostbyname(host);
	if(he != 0 && he->h_addr_list[0]) {
		p = (uchar*)he->h_addr_list[0];
		sprint(buf, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);
	} else
		strcpy(buf, host);

	return strdup(buf);
}

int
so_getservbyname(char *service, char *net, char *port)
{
	struct servent *s;

	s = getservbyname(service, net);
	if(s == 0)
		return -1;

	sprint(port, "%d", nhgets(&s->s_port));
	return 0;
}

void
hnputl(void *p, unsigned long v)
{
	unsigned char *a;

	a = p;
	a[0] = v>>24;
	a[1] = v>>16;
	a[2] = v>>8;
	a[3] = v;
}

void
hnputs(void *p, unsigned short v)
{
	unsigned char *a;

	a = p;
	a[0] = v>>8;
	a[1] = v;
}

unsigned long
nhgetl(void *p)
{
	unsigned char *a;
	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

unsigned short
nhgets(void *p)
{
	unsigned char *a;
	a = p;
	return (a[0]<<8)|(a[1]<<0);
}

#define CLASS(p) ((*(unsigned char*)(p))>>6)

unsigned long
parseip(char *to, char *from)
{
	int i;
	char *p;

	p = from;
	memset(to, 0, 4);
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoul(p, &p, 0);
		if(*p == '.')
			p++;
	}
	switch(CLASS(to)){
	case 0:	/* class A - 1 byte net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 byte net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
	return nhgetl(to);
}
