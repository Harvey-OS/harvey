/*
 * User-level PPP over Ethernet (PPPoE) client.
 * See RFC 2516
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

void dumppkt(uchar*);
uchar *findtag(uchar*, int, int*, int);
void hexdump(uchar*, int);
int malformed(uchar*, int, int);
int pppoe(char*);
void execppp(int);

int alarmed;
int debug;
int sessid;
char *keyspec;
int primary;
char *pppnetmtpt;
char *acname;
char *pppname = "/bin/ip/ppp";
char *srvname = "";
char *wantac;
uchar *cookie;
int cookielen;
uchar etherdst[6];
int mtu = 1492;

void
usage(void)
{
	fprint(2, "usage: pppoe [-Pd] [-A acname] [-S srvname] [-k keyspec] [-m mtu] [-x pppnet] [ether0]\n");
	exits("usage");
}

int
catchalarm(void *a, char *msg)
{
	USED(a);

	if(strstr(msg, "alarm")){
		alarmed = 1;
		return 1;
	}
	if(debug)
		fprint(2, "note rcved: %s\n", msg);
	return 0;
}

void
main(int argc, char **argv)
{
	int fd;
	char *dev;

	ARGBEGIN{
	case 'A':
		wantac = EARGF(usage());
		break;
	case 'P':
		primary = 1;
		break;
	case 'S':
		srvname = EARGF(usage());
		break;
	case 'd':
		debug++;
		break;
	case 'm':
		mtu = atoi(EARGF(usage()));
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'x':
		pppnetmtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	switch(argc){
	default:
		usage();
	case 0:
		dev = "ether0";
		break;
	case 1:
		dev = argv[0];
		break;
	}

	fmtinstall('E', eipfmt);

	atnotify(catchalarm, 1);
	fd = pppoe(dev);
	execppp(fd);
}

typedef struct Etherhdr Etherhdr;
struct Etherhdr {
	uchar dst[6];
	uchar src[6];
	uchar type[2];
};

enum {
	EtherHdrSz = 6+6+2,
	EtherMintu = 60,

	EtherPppoeDiscovery = 0x8863,
	EtherPppoeSession = 0x8864,
};

uchar etherbcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

int
etherhdr(uchar *pkt, uchar *dst, int type)
{
	Etherhdr *eh;

	eh = (Etherhdr*)pkt;
	memmove(eh->dst, dst, sizeof(eh->dst));
	hnputs(eh->type, type);
	return EtherHdrSz;
}

typedef struct Pppoehdr Pppoehdr;
struct Pppoehdr {
	uchar verstype;
	uchar code;
	uchar sessid[2];
	uchar length[2];	/* of payload */
};

enum {
	PppoeHdrSz = 1+1+2+2,
	Hdr = EtherHdrSz+PppoeHdrSz,
};

enum {
	VersType = 0x11,

	/* Discovery codes */
	CodeDiscInit = 0x09,	/* discovery init */
	CodeDiscOffer = 0x07,	/* discovery offer */
	CodeDiscReq = 0x19,	/* discovery request */
	CodeDiscSess = 0x65,	/* session confirmation */

	/* Session codes */
	CodeSession = 0x00,
};

int
pppoehdr(uchar *pkt, int code, int sessid)
{
	Pppoehdr *ph;

	ph = (Pppoehdr*)pkt;
	ph->verstype = VersType;
	ph->code = code;
	hnputs(ph->sessid, sessid);
	return PppoeHdrSz;
}

typedef struct Taghdr Taghdr;
struct Taghdr {
	uchar type[2];
	uchar length[2];	/* of value */
};

enum {
	TagEnd = 0x0000,		/* end of tag list */
	TagSrvName = 0x0101,	/* service name */
	TagAcName = 0x0102,	/* access concentrator name */
	TagHostUniq = 0x0103,	/* nonce */
	TagAcCookie = 0x0104,	/* a.c. cookie */
	TagVendSpec = 0x0105,	/* vendor specific */
	TagRelaySessId = 0x0110,	/* relay session id */
	TagSrvNameErr = 0x0201,	/* service name error (ascii) */
	TagAcSysErr = 0x0202,	/* a.c. system error */
};

int
tag(uchar *pkt, int type, void *value, int nvalue)
{
	Taghdr *h;

	h = (Taghdr*)pkt;
	hnputs(h->type, type);
	hnputs(h->length, nvalue);
	memmove(pkt+4, value, nvalue);
	return 4+nvalue;
}

/* PPPoE Active Discovery Initiation */
int
padi(uchar *pkt)
{
	int sz, tagoff;
	uchar *length;

	sz = 0;
	sz += etherhdr(pkt+sz, etherbcast, EtherPppoeDiscovery);
	sz += pppoehdr(pkt+sz, CodeDiscInit, 0x0000);
	length = pkt+sz-2;
	tagoff = sz;
	sz += tag(pkt+sz, TagSrvName, srvname, strlen(srvname));
	hnputs(length, sz-tagoff);
	return sz;
}

/* PPPoE Active Discovery Request */
int
padr(uchar *pkt)
{
	int sz, tagoff;
	uchar *length;

	sz = 0;
	sz += etherhdr(pkt+sz, etherdst, EtherPppoeDiscovery);
	sz += pppoehdr(pkt+sz, CodeDiscReq, 0x0000);
	length = pkt+sz-2;
	tagoff = sz;
	sz += tag(pkt+sz, TagSrvName, srvname, strlen(srvname));
	sz += tag(pkt+sz, TagAcName, acname, strlen(acname));
	if(cookie)
		sz += tag(pkt+sz, TagAcCookie, cookie, cookielen);
	hnputs(length, sz-tagoff);
	return sz;
}

void
ewrite(int fd, void *buf, int nbuf)
{
	char e[ERRMAX], path[64];

	if(write(fd, buf, nbuf) != nbuf){
		rerrstr(e, sizeof e);
		strcpy(path, "unknown");
		fd2path(fd, path, sizeof path);
		sysfatal("write %d to %s: %s", nbuf, path, e);
	}
}

void*
emalloc(long n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		sysfatal("out of memory");
	return v;
}

int
aread(int timeout, int fd, void *buf, int nbuf)
{
	int n;

	alarmed = 0;
	alarm(timeout);
	n = read(fd, buf, nbuf);
	alarm(0);
	if(alarmed)
		return -1;
	if(n < 0)
		sysfatal("read: %r");
	if(n == 0)
		sysfatal("short read");
	return n;
}

int
pktread(int timeout, int fd, void *buf, int nbuf, int (*want)(uchar*))
{
	int n, t2;
	n = -1;
	for(t2=timeout; t2<16000; t2*=2){
		while((n = aread(t2, fd, buf, nbuf)) > 0){
			if(malformed(buf, n, EtherPppoeDiscovery)){
				if(debug)
					fprint(2, "dropping pkt: %r\n");
				continue;
			}
			if(debug)
				dumppkt(buf);
			if(!want(buf)){
				if(debug)
					fprint(2, "dropping unwanted pkt: %r\n");
				continue;
			}
			break;
		}
		if(n > 0)
			break;
	}
	return n;
}

int
bad(char *reason)
{
	werrstr(reason);
	return 0;
}

void*
copy(uchar *s, int len)
{
	uchar *v;

	v = emalloc(len+1);
	memmove(v, s, len);
	v[len] = '\0';
	return v;
}

void
clearstate(void)
{
	sessid = -1;
	free(acname);
	acname = nil;
	free(cookie);
	cookie = nil;
}

int
wantoffer(uchar *pkt)
{
	int i, len;
	uchar *s;
	Etherhdr *eh;
	Pppoehdr *ph;

	eh = (Etherhdr*)pkt;
	ph = (Pppoehdr*)(pkt+EtherHdrSz);

	if(ph->code != CodeDiscOffer)
		return bad("not an offer");
	if(nhgets(ph->sessid) != 0x0000)
		return bad("bad session id");

	for(i=0;; i++){
		if((s = findtag(pkt, TagSrvName, &len, i)) == nil)
			return bad("no matching service name");
		if(len == strlen(srvname) && memcmp(s, srvname, len) == 0)
			break;
	}

	if((s = findtag(pkt, TagAcName, &len, 0)) == nil)
		return bad("no ac name");
	acname = copy(s, len);
	if(wantac && strcmp(acname, wantac) != 0){
		free(acname);
		return bad("wrong ac name");
	}

	if(s = findtag(pkt, TagAcCookie, &len, 0)){
		cookie = copy(s, len);
		cookielen = len;
	}
	memmove(etherdst, eh->src, sizeof etherdst);
	return 1;
}

int
wantsession(uchar *pkt)
{
	int len;
	uchar *s;
	Pppoehdr *ph;

	ph = (Pppoehdr*)(pkt+EtherHdrSz);

	if(ph->code != CodeDiscSess)
		return bad("not a session confirmation");
	if(nhgets(ph->sessid) == 0x0000)
		return bad("bad session id");
	if(findtag(pkt, TagSrvName, &len, 0) == nil)
		return bad("no service name");
	if(findtag(pkt, TagSrvNameErr, &len, 0))
		return bad("service name error");
	if(findtag(pkt, TagAcSysErr, &len, 0))
		return bad("ac system error");

	/*
	 * rsc said: ``if there is no -S option given, the current code
	 * waits for an offer with service name == "".
	 * that's silly.  it should take the first one it gets.''
	 */
	if(srvname[0] != '\0') {
		if((s = findtag(pkt, TagSrvName, &len, 0)) == nil)
			return bad("no matching service name");
		if(len != strlen(srvname) || memcmp(s, srvname, len) != 0)
			return bad("no matching service name");
	}
	sessid = nhgets(ph->sessid);
	return 1;
}

int
pppoe(char *ether)
{
	char buf[64];
	uchar pkt[1520];
	int dfd, p[2], n, sfd, sz, timeout;
	Pppoehdr *ph;

	ph = (Pppoehdr*)(pkt+EtherHdrSz);
	snprint(buf, sizeof buf, "%s!%d", ether, EtherPppoeDiscovery);
	if((dfd = dial(buf, nil, nil, nil)) < 0)
		sysfatal("dial %s: %r", buf);

	snprint(buf, sizeof buf, "%s!%d", ether, EtherPppoeSession);
	if((sfd = dial(buf, nil, nil, nil)) < 0)
		sysfatal("dial %s: %r", buf);

	for(timeout=250; timeout<16000; timeout*=2){
		clearstate();
		memset(pkt, 0, sizeof pkt);
		sz = padi(pkt);
		if(debug)
			dumppkt(pkt);
		if(sz < EtherMintu)
			sz = EtherMintu;
		ewrite(dfd, pkt, sz);

		if(pktread(timeout, dfd, pkt, sizeof pkt, wantoffer) < 0)
			continue;

		memset(pkt, 0, sizeof pkt);
		sz = padr(pkt);
		if(debug)
			dumppkt(pkt);
		if(sz < EtherMintu)
			sz = EtherMintu;
		ewrite(dfd, pkt, sz);

		if(pktread(timeout, dfd, pkt, sizeof pkt, wantsession) < 0)
			continue;

		break;
	}
	if(sessid < 0)
		sysfatal("could not establish session");

	rfork(RFNOTEG);
	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	switch(fork()){
	case -1:
		sysfatal("fork: %r");
	default:
		break;
	case 0:
		close(p[1]);
		while((n = read(p[0], pkt+Hdr, sizeof pkt-Hdr)) > 0){
			etherhdr(pkt, etherdst, EtherPppoeSession);
			pppoehdr(pkt+EtherHdrSz, 0x00, sessid);
			hnputs(pkt+Hdr-2, n);
			sz = Hdr+n;
			if(debug > 1){
				dumppkt(pkt);
				hexdump(pkt, sz);
			}
			if(sz < EtherMintu)
				sz = EtherMintu;
			if(write(sfd, pkt, sz) < 0){
				if(debug)
					fprint(2, "write to ether failed: %r");
				_exits(nil);
			}
		}
		_exits(nil);
	}

	switch(fork()){
	case -1:
		sysfatal("fork: %r");
	default:
		break;
	case 0:
		close(p[1]);
		while((n = read(sfd, pkt, sizeof pkt)) > 0){
			if(malformed(pkt, n, EtherPppoeSession)
			|| ph->code != 0x00 || nhgets(ph->sessid) != sessid){
				if(debug)
					fprint(2, "malformed session pkt: %r\n");
				if(debug)
					dumppkt(pkt);
				continue;
			}
			if(write(p[0], pkt+Hdr, nhgets(ph->length)) < 0){
				if(debug)
					fprint(2, "write to ppp failed: %r\n");
				_exits(nil);
			}
		}
		_exits(nil);
	}
	close(p[0]);
	return p[1];
}

void
execppp(int fd)
{
	char *argv[16];
	int argc;
	char smtu[10];

	argc = 0;
	argv[argc++] = pppname;
	snprint(smtu, sizeof(smtu), "-m%d", mtu);
	argv[argc++] = smtu;
	argv[argc++] = "-F";
	if(debug)
		argv[argc++] = "-d";
	if(primary)
		argv[argc++] = "-P";
	if(pppnetmtpt){
		argv[argc++] = "-x";
		argv[argc++] = pppnetmtpt;
	}
	if(keyspec){
		argv[argc++] = "-k";
		argv[argc++] = keyspec;
	}
	argv[argc] = nil;

	dup(fd, 0);
	dup(fd, 1);
	exec(pppname, argv);
	sysfatal("exec: %r");
}

uchar*
findtag(uchar *pkt, int tagtype, int *plen, int skip)
{
	int len, sz, totlen;
	uchar *tagdat, *v;
	Etherhdr *eh;
	Pppoehdr *ph;
	Taghdr *t;

	eh = (Etherhdr*)pkt;
	ph = (Pppoehdr*)(pkt+EtherHdrSz);
	tagdat = pkt+Hdr;

	if(nhgets(eh->type) != EtherPppoeDiscovery)
		return nil;
	totlen = nhgets(ph->length);

	sz = 0;
	while(sz+4 <= totlen){
		t = (Taghdr*)(tagdat+sz);
		v = tagdat+sz+4;
		len = nhgets(t->length);
		if(sz+4+len > totlen)
			break;
		if(nhgets(t->type) == tagtype && skip-- == 0){
			*plen = len;
			return v;
		}
		sz += 2+2+len;
	}
	return nil;	
}

void
dumptags(uchar *tagdat, int ntagdat)
{
	int i,len, sz;
	uchar *v;
	Taghdr *t;

	sz = 0;
	while(sz+4 <= ntagdat){
		t = (Taghdr*)(tagdat+sz);
		v = tagdat+sz+2+2;
		len = nhgets(t->length);
		if(sz+4+len > ntagdat)
			break;
		fprint(2, "\t0x%x %d: ", nhgets(t->type), len);
		switch(nhgets(t->type)){
		case TagEnd:
			fprint(2, "end of tag list\n");
			break;
		case TagSrvName:
			fprint(2, "service '%.*s'\n", len, (char*)v);
			break;
		case TagAcName:
			fprint(2, "ac '%.*s'\n", len, (char*)v);
			break;
		case TagHostUniq:
			fprint(2, "nonce ");
		Hex:
			for(i=0; i<len; i++)
				fprint(2, "%.2ux", v[i]);
			fprint(2, "\n");
			break;
		case TagAcCookie:
			fprint(2, "ac cookie ");
			goto Hex;
		case TagVendSpec:
			fprint(2, "vend spec ");
			goto Hex;
		case TagRelaySessId:
			fprint(2, "relay ");
			goto Hex;
		case TagSrvNameErr:
			fprint(2, "srverr '%.*s'\n", len, (char*)v);
			break;
		case TagAcSysErr:
			fprint(2, "syserr '%.*s'\n", len, (char*)v);
			break;
		}
		sz += 2+2+len;
	}
	if(sz != ntagdat)
		fprint(2, "warning: only dumped %d of %d bytes\n", sz, ntagdat);
}

void
dumppkt(uchar *pkt)
{
	int et;
	Etherhdr *eh;
	Pppoehdr *ph;

	eh = (Etherhdr*)pkt;
	ph = (Pppoehdr*)(pkt+EtherHdrSz);
	et = nhgets(eh->type);

	fprint(2, "%E -> %E type 0x%x\n", 
		eh->src, eh->dst, et);
	switch(et){
	case EtherPppoeDiscovery:
	case EtherPppoeSession:
		fprint(2, "\tvers %d type %d code 0x%x sessid 0x%x length %d\n",
			ph->verstype>>4, ph->verstype&15,
			ph->code, nhgets(ph->sessid), nhgets(ph->length));
		if(et == EtherPppoeDiscovery)
			dumptags(pkt+Hdr, nhgets(ph->length));
	}
}

int
malformed(uchar *pkt, int n, int wantet)
{
	int et;
	Etherhdr *eh;
	Pppoehdr *ph;

	eh = (Etherhdr*)pkt;
	ph = (Pppoehdr*)(pkt+EtherHdrSz);

	if(n < Hdr || n < Hdr+nhgets(ph->length)){
		werrstr("packet too short %d != %d", n, Hdr+nhgets(ph->length));
		return 1;
	}

	et = nhgets(eh->type);
	if(et != wantet){
		werrstr("wrong ethernet packet type 0x%x != 0x%x", et, wantet);
		return 1;
	}

	return 0;
}

void
hexdump(uchar *a, int na)
{
	int i;
	char buf[80];

	buf[0] = '\0';
	for(i=0; i<na; i++){
		sprint(buf+strlen(buf), " %.2ux", a[i]);
		if(i%16 == 7)
			sprint(buf+strlen(buf), " --");
		if(i%16==15){
			sprint(buf+strlen(buf), "\n");
			write(2, buf, strlen(buf));
			buf[0] = 0;
		}
	}
	if(i%16){
		sprint(buf+strlen(buf), "\n");
		write(2, buf, strlen(buf));
	}
}
