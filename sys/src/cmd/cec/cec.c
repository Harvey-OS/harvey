/*
 * cec — coraid ethernet console
 * Copyright © Coraid, Inc. 2006-2008.
 * All Rights Reserved.
 */
#include <u.h>
#include <libc.h>
#include <ip.h>		/* really! */
#include <ctype.h>
#include "cec.h"

enum {
	Tinita		= 0,
	Tinitb,
	Tinitc,
	Tdata,
	Tack,
	Tdiscover,
	Toffer,
	Treset,

	Hdrsz		= 18,
	Eaddrlen	= 6,
};

typedef struct{
	uchar	ea[Eaddrlen];
	int	major;
	char	name[28];
} Shelf;

int 	conn(int);
void 	gettingkilled(int);
int 	pickone(void);
void 	probe(void);
void	sethdr(Pkt *, int);
int	shelfidx(void);

Shelf	*con;
Shelf	tab[1000];

char	*host;
char	*srv;
char	*svc;

char	pflag;

int	ntab;
int	shelf = -1;

uchar	bcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uchar	contag;
uchar 	esc = '';
uchar	ea[Eaddrlen];
uchar	unsetea[Eaddrlen];

extern 	int fd;		/* set in netopen */

void
post(char *srv, int fd)
{
	char buf[32];
	int f;

	if((f = create(srv, OWRITE, 0666)) == -1)
		sysfatal("create %s: %r", srv);
	snprint(buf, sizeof buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		sysfatal("write %s: %r", srv);
	close(f);
}

void
dosrv(char *s)
{
	int p[2];

	if(pipe(p) < 0)
		sysfatal("pipe: %r");
	if (srv[0] != '/')
		svc = smprint("/srv/%s", s);
	else
		svc = smprint("%s", s);
	post(svc, p[0]);
	close(p[0]);
	dup(p[1], 0);
	dup(p[1], 1);

	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		break;
	default:
		exits("");
	}
	close(2);
}

void
usage(void)
{
	fprint(2, "usage: cec [-dp] [-c esc] [-e ea] [-h host] [-s shelf] "
		"[-S srv] interface\n");
	exits0("usage");
}

void
catch(void*, char *note)
{
	if(strcmp(note, "alarm") == 0)
		noted(NCONT);
	noted(NDFLT);
}

int
nilea(uchar *ea)
{
	return memcmp(ea, unsetea, Eaddrlen) == 0;
}

void
main(int argc, char **argv)
{
	int r, n;

	ARGBEGIN{
	case 'S':
		srv = EARGF(usage());
		break;
	case 'c':
		esc = tolower(*(EARGF(usage()))) - 'a' + 1;
		if(esc == 0 || esc >= ' ')
			usage();
		break;
	case 'd':
		debug++;
		break;
	case 'e':
		if(parseether(ea, EARGF(usage())) == -1)
			usage();
		pflag = 1;
		break;
	case 'h':
		host = EARGF(usage());
		break;
	case 'p':
		pflag = 1;
		break;
	case 's':
		shelf = atoi(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND
	if(argc == 0)
		*argv = "/net/ether0";
	else if(argc != 1)
		usage();

	fmtinstall('E', eipfmt);
	if(srv != nil)
		dosrv(srv);
	r = netopen(*argv);
	if(r == -1){
		fprint(2, "cec: can't netopen %s\n", *argv);
		exits0("open");
	}
	notify(catch);
	probe();
	for(;;){
		n = 0;
		if(shelf == -1 && host == 0 && nilea(ea))
			n = pickone();
		rawon();
		conn(n);
		rawoff();
		if(pflag == 0){
			if(shelf != -1)
				exits0("shelf not found");
			if(host)
				exits0("host not found");
			if(!nilea(ea))
				exits0("ea not found");
		} else if(shelf != -1 || host || !nilea(ea))
			exits0("");
	}
}

void
timewait(int ms)
{
	alarm(ms);
}

int
didtimeout(void)
{
	char buf[ERRMAX];

	rerrstr(buf, sizeof buf);
	if(strcmp(buf, "interrupted") == 0){
		werrstr(buf, 0);
		return 1;
	}
	return 0;
}

ushort
htons(ushort h)
{
	ushort n;
	uchar *p;

	p = (uchar*)&n;
	p[0] = h >> 8;
	p[1] = h;
	return n;
}

ushort
ntohs(int h)
{
	ushort n;
	uchar *p;

	n = h;
	p = (uchar*)&n;
	return p[0] << 8 | p[1];
}

int
tcmp(void *a, void *b)
{
	Shelf *s, *t;
	int d;

	s = a;
	t = b;
	d = s->major - t->major;
	if(d == 0)
		d = strcmp(s->name, t->name);
	if(d == 0)
		d = memcmp(s->ea, t->ea, Eaddrlen);
	return d;
}

void
probe(void)
{
	char *sh, *other;
	int n;
	Pkt q;
	Shelf *p;

	do {
		ntab = 0;
		memset(q.dst, 0xff, Eaddrlen);
		memset(q.src, 0, Eaddrlen);
		q.etype = htons(Etype);
		q.type = Tdiscover;
		q.len = 0;
		q.conn = 0;
		q.seq = 0;
		netsend(&q, 60);
		timewait(Iowait);
		while((n = netget(&q, sizeof q)) >= 0){
			if((n <= 0 && didtimeout()) || ntab == nelem(tab))
				break;
			if(n < 60 || q.len == 0 || q.type != Toffer)
				continue;
			q.data[q.len] = 0;
			sh = strtok((char *)q.data, " \t");
			if(sh == nil)
				continue;
			if(!nilea(ea) && memcmp(ea, q.src, Eaddrlen) != 0)
				continue;
			if(shelf != -1 && atoi(sh) != shelf)
				continue;
			other = strtok(nil, "\x1");
			if(other == 0)
				other = "";
			if(host && strcmp(host, other) != 0)
				continue;
			p = tab + ntab++;
			memcpy(p->ea, q.src, Eaddrlen);
			p->major = atoi(sh);
			p->name[0] = 0;
			if(p->name)
				snprint(p->name, sizeof p->name, "%s", other);
		}
		alarm(0);
	} while (ntab == 0 && pflag);
	if(ntab == 0){
		fprint(2, "none found.\n");
		exits0("none found");
	}
	qsort(tab, ntab, sizeof tab[0], tcmp);
}

void
showtable(void)
{
	int i;

	for(i = 0; i < ntab; i++)
		print("%2d   %5d %E %s\n", i, tab[i].major, tab[i].ea, tab[i].name);
}

int
pickone(void)
{
	char buf[80];
	int n, i;

	for(;;){
		showtable();
		print("[#qp]: ");
		switch(n = read(0, buf, sizeof buf)){
		case 1:
			if(buf[0] == '\n')
				continue;
			/* fall through */
		case 2:
			if(buf[0] == 'p'){
				probe();
				break;
			}
			if(buf[0] == 'q')
				/* fall through */
		case 0:
		case -1:
				exits0(0);
			break;
		}
		if(isdigit(buf[0])){
			buf[n] = 0;
			i = atoi(buf);
			if(i >= 0 && i < ntab)
				break;
		}
	}
	return i;
}

void
sethdr(Pkt *pp, int type)
{
	memmove(pp->dst, con->ea, Eaddrlen);
	memset(pp->src, 0, Eaddrlen);
	pp->etype = htons(Etype);
	pp->type = type;
	pp->len = 0;
	pp->conn = contag;
}

void
ethclose(void)
{
	static Pkt msg;

	sethdr(&msg, Treset);
	timewait(Iowait);
	netsend(&msg, 60);
	alarm(0);
	con = 0;
}

int
ethopen(void)
{
	Pkt tpk, rpk;
	int i, n;

	contag = (getpid() >> 8) ^ (getpid() & 0xff);
	sethdr(&tpk, Tinita);
	sethdr(&rpk, 0);
	for(i = 0; i < 3 && rpk.type != Tinitb; i++){
		netsend(&tpk, 60);
		timewait(Iowait);
		n = netget(&rpk, 1000);
		alarm(0);
		if(n < 0)
			return -1;
	}
	if(rpk.type != Tinitb)
		return -1;
	sethdr(&tpk, Tinitc);
	netsend(&tpk, 60);
	return 0;
}

char
escape(void)
{
	char buf[64];
	int r;

	for(;;){
		fprint(2, ">>> ");
		buf[0] = '.';
		rawoff();
		r = read(0, buf, sizeof buf - 1);
		rawon();
		if(r == -1)
			exits0("kbd: %r");
		switch(buf[0]){
		case 'i':
		case 'q':
		case '.':
			return buf[0];
		}
		fprint(2, "	(q)uit, (i)nterrupt, (.)continue\n");
	}
}

/*
 * this is a bit too aggressive.  it really needs to replace only \n\r with \n.
 */
static uchar crbuf[256];

void
nocrwrite(int fd, uchar *buf, int n)
{
	int i, j, c;

	j = 0;
	for(i = 0; i < n; i++)
		if((c = buf[i]) != '\r')
			crbuf[j++] = c;
	write(fd, crbuf, j);
}

int
doloop(void)
{
	int unacked, retries, set[2];
	uchar c, tseq, rseq;
	uchar ea[Eaddrlen];
	Pkt tpk, spk;
	Mux *m;

	memmove(ea, con->ea, Eaddrlen);
	retries = 0;
	unacked = 0;
	tseq = 0;
	rseq = -1;
	set[0] = 0;
	set[1] = fd;
top:
	if ((m = mux(set)) == 0)
		exits0("mux: %r");
	for (; ; )
		switch (muxread(m, &spk)) {
		case -1:
			if (unacked == 0)
				break;
			if (retries-- == 0) {
				fprint(2, "Connection timed out\n");
				muxfree(m);
				return 0;
			}
			netsend(&tpk, Hdrsz + unacked);
			break;
		case Fkbd:
			c = spk.data[0];
			if (c == esc) {
				muxfree(m);
				switch (escape()) {
				case 'q':
					tpk.len = 0;
					tpk.type = Treset;
					netsend(&tpk, 60);
					return 0;
				case '.':
					goto top;
				case 'i':
					if ((m = mux(set)) == 0)
						exits0("mux: %r");
					break;
				}
			}
			sethdr(&tpk, Tdata);
			memcpy(tpk.data, spk.data, spk.len);
			tpk.len = spk.len;
			tpk.seq = ++tseq;
			unacked = spk.len;
			retries = 2;
			netsend(&tpk, Hdrsz + spk.len);
			break;
		case Fcec:
			if (memcmp(spk.src, ea, Eaddrlen) != 0 ||
			    ntohs(spk.etype) != Etype)
				continue;
			if (spk.type == Toffer &&
			    memcmp(spk.dst, bcast, Eaddrlen) != 0) {
				muxfree(m);
				return 1;
			}
			if (spk.conn != contag)
				continue;
			switch (spk.type) {
			case Tdata:
				if (spk.seq == rseq)
					break;
				nocrwrite(1, spk.data, spk.len);
				memmove(spk.dst, spk.src, Eaddrlen);
				memset(spk.src, 0, Eaddrlen);
				spk.type = Tack;
				spk.len = 0;
				rseq = spk.seq;
				netsend(&spk, 60);
				break;
			case Tack:
				if (spk.seq == tseq)
					unacked = 0;
				break;
			case Treset:
				muxfree(m);
				return 1;
			}
			break;
		case Ffatal:
			muxfree(m);
			fprint(2, "kbd read error\n");
			exits0("fatal");
		}
}

int
conn(int n)
{
	int r;

	for(;;){
		if(con)
			ethclose();
		con = tab + n;
		if(ethopen() < 0){
			fprint(2, "connection failed\n");
			return 0;
		}
		r = doloop();
		if(r <= 0)
			return r;
	}
}

void
exits0(char *s)
{
	if(con != nil)
		ethclose();
	rawoff();
	if(svc != nil)
		remove(svc);
	exits(s);
}
