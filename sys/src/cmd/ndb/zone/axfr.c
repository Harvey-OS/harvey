/*
 * axfr - fetch domain from ns via tcp, print in ndb format
 *
 * Steve Simon and Geoff Collyer
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ctype.h>

enum {
	Fqdnmax	= 255,

	Cin	= 1,	/* internet; other classes are effectively unused */

	Ta	= 1,	/* a host address */
	Tns	= 2,	/* an authoritative name server */
	Tmd	= 3,	/* a mail destination (Obsolete - use MX) */
	Tmf	= 4,	/* a mail forwarder (Obsolete - use MX) */
	Tcname	= 5,	/* the canonical name for an alias */
	Tsoa	= 6,	/* marks the start of a zone of authority */
	Tmb	= 7,	/* a mailbox domain name (EXPERIMENTAL) */
	Tmg	= 8,	/* a mail group member (EXPERIMENTAL) */
	Tmr	= 9,	/* a mail rename domain name (EXPERIMENTAL) */
	Tnull	= 10,	/* a null RR (EXPERIMENTAL) */
	Twks	= 11,	/* a well known service description */
	Tptr	= 12,	/* a domain name pointer */
	Thinfo	= 13,	/* host information */
	Tminfo	= 14,	/* mailbox or mail list information */
	Tmx	= 15,	/* mail exchange */
	Ttxt	= 16,	/* text strings */
	Trp	= 17,	/* for Responsible Person */
	Tafsdb	= 18,	/* for AFS Data Base location */
	Tx25	= 19,	/* for X.25 PSDN address */
	Tisdn	= 20,	/* for ISDN address */
	Trt	= 21,	/* for Route Through */
	Tnsap	= 22,	/* for NSAP address, NSAP style A record */
	Tnsap_ptr = 23,	/* for domain name pointer, NSAP style */
	Tsig	= 24,	/* for security signature */
	Tkey	= 25,	/* for security key */
	Tpx	= 26,	/* X.400 mail mapping information */
	Tgpos	= 27,	/* Geographical Position */
	Taaaa	= 28,	/* IP6 Address */
	Tloc	= 29,	/* Location Information */
	Tnxt	= 30,	/* Next Domain (obsolete) */
	Teid	= 31,	/* Endpoint Identifier */
	Tnimloc	= 32,	/* Nimrod Locator */
	Tsrv	= 33,	/* Server Selection */
	Tatma	= 34,	/* ATM Address */
	Tnaptr	= 35,	/* Naming Authority Pointer */
	Tkx	= 36,	/* Key Exchanger */
	Tcert	= 37,
	Ta6	= 38,
	Tdname	= 39,
	Tsink	= 40,
	Topt	= 41,
	Tapl	= 42,
	Tds	= 43,	/* Delegation Signer */
	Tsshfp	= 44,	/* SSH Key Fingerprint */
	Trrsig	= 46,
	Tnsec	= 47,
	Tdnskey	= 48,
	Tuinfo	= 100,
	Tuid	= 101,
	Tgid	= 102,
	Tunspec	= 103,
	Taddrs	= 248,
	Ttkey	= 249,
	Ttsig	= 250,	/* Transaction Signature */
	Tixfr	= 251,	/* Incremental transfer */
	Taxfr	= 252,	/* transfer of an entire zone */
	Tmailb	= 253,	/* mailbox-related RRs (MB, MG or MR) */
	Tmaila	= 254,	/* mail agent RRs (Obsolete - see MX) */
	Tall	= 255,	/* A request for all records */
	Twins	= 0xff01,	/* MS WINS server */
	Twinsr	= 0xff02,	/* MS wins reverse lookup */
};

typedef struct {
	uchar	*base;
	uchar	*ptr;
	int	len;
} Scan;

typedef struct {
	int	nques;
	int	nans;
	int	naut;
	int	nadd;
} Msghdr;

static char *Typestr[] = {
	[Ta]		"a",
	[Tns]		"ns",
	[Tmd]		"md",
	[Tmf]		"mf",
	[Tcname]	"cname",
	[Tsoa]		"soa",
	[Tmb]		"mb",
	[Tmg]		"mg",
	[Tmr]		"mr",
	[Tnull]		"null",
	[Twks]		"wks",
	[Tptr]		"ptr",
	[Thinfo]	"hinfo",
	[Tminfo]	"minfo",
	[Tmx]		"mx",
	[Ttxt]		"txt",
	[Trp]		"rp",
	[Tafsdb]	"afsdb",
	[Tx25]		"x25",
	[Tisdn]		"isdn",
	[Trt]		"rt",
	[Tnsap]		"nsap",
	[Tnsap_ptr]	"nsap_ptr",
	[Tsig]		"sig",
	[Tkey]		"key",
	[Tpx]		"px",
	[Tgpos]		"gpos",
	[Taaaa]		"aaaa",
	[Tloc]		"loc",
	[Tnxt]		"nxt",
	[Teid]		"eid",
	[Tnimloc]	"nimloc",
	[Tsrv]		"srv",
	[Tatma]		"atma",
	[Tnaptr]	"naptr",
	[Tkx]		"kx",
	[Tcert]		"cert",
	[Ta6]		"a6",
	[Tdname]	"dname",
	[Tsink]		"sink",
	[Topt]		"opt",
	[Tapl]		"apl",
	[Tds]		"ds",
	[Tsshfp]	"sshfp",
	[Trrsig]	"rrsig",
	[Tnsec]		"nsec",
	[Tdnskey]	"dnskey",
	[Tuinfo]	"uinfo",
	[Tuid]		"uid",
	[Tgid]		"gid",
	[Tunspec]	"unspec",
	[Taddrs]	"addrs",
	[Ttkey]		"tkey",
	[Ttsig]		"tsig",
	[Tixfr]		"ixfr",
	[Taxfr]		"axfr",
	[Tmailb]	"mailb",
	[Tmaila]	"maila",
	[Tall]		"all",
};

static int debug;
static Biobuf *bo;

void
usage(void)
{
	fprint(2, "usage: %s [-x netmtpt] [-d] nameserver domainname\n", argv0);
	exits("usage");
}

void
ding(void *, char *msg)
{
	if(strstr(msg, "alarm") != nil)
		noted(NCONT);
	else
		noted(NDFLT);
}

int
g8(uchar **p)
{
	return *(*p)++;
}

ushort
g16(uchar **p)
{
	*p += 2;
	return (*p)[-2] << 8 | (*p)[-1];
}

int
g32(uchar **p)
{
	*p += 4;
	return (*p)[-4] << 24 | (*p)[-3] << 16 | (*p)[-2] << 8 | (*p)[-1];
}

void
gname(uchar **p, uchar *buf, char *s)
{
	char *last = s;
	uchar *q;
	int n;

	while (n = g8(p)){
		if(n & 0xc0){
			(*p)--;
			n = g16(p);
			q = buf + (n & 0x3fff) + 2;  /* +2 to skip packet len */
			gname(&q, buf, s);
			return;
		}
		while (**p && n--)
			*s++ = *(*p)++;
		last = s;
		*s++ = '.';
	}
	*last = 0;
}

void
skip(uchar **p, int len)
{
	if(debug)
		fprint(2, "skipped %d bytes at end of record\n", len);
	*p += len;
}

void *
gmem(uchar **p, int len)
{
	char *s;

	assert(s = malloc(len + 1));
	memmove(s, *p, len);
	*p += len;
	s[len] = '\0';		/* unnecessary courtesy */
	return s;
}

void
p8(uchar **p, int n)
{
	*(*p)++ = n;
}

void
p16(uchar **p, ushort n)
{
	*(*p)++ = n >> 8;
	*(*p)++ = n;
}

void
p32(uchar **p, ulong n)
{
	*(*p)++ = n >> 24;
	*(*p)++ = n >> 16;
	*(*p)++ = n >> 8;
	*(*p)++ = n;
}

void
pmem(uchar **p, void *v, int len)
{
	memmove(*p, v, len);
	*p += len;
}

void
pname(uchar **p, char *s)
{
	uchar *len;

	while (*s){
		len = (*p)++;
		while(*s && *s != '.')
			*(*p)++ = *s++;
		*len = (*p - len) - 1;
		if(*s == '.')
			s++;
	}
	*(*p)++ = 0;
}

void
xd(void *p, int l)
{
	int i;
	uchar *buf = p;

	Bprint(bo, "\n%-4x ", 0);
	for (i = 0; i < l && i < 128; i++){
		if(isascii(buf[i]) && isprint(buf[i]))
			Bprint(bo, " %c ", buf[i]);
		else
			Bprint(bo, "%02x ", buf[i]);
		if(i != 0 && (i % 16) == 0)
			Bprint(bo, "\n%-4x ", i);
	}
	Bprint(bo, "\n");
	Bflush(bo);
}

void
pr(uchar **pp, uchar *buf, int vlen, char *name, int type)
{
	int n, i, j, k;
	uchar *ad = nil;
	char *p = nil, *a[128], s[Fqdnmax +1];

	switch(type){
	case Ta:
		ad = gmem(pp, vlen);
		Bprint(bo, "dom=%s ip=%V\n", name, ad);
		break;
	case Taaaa:
		ad = gmem(pp, vlen);
		Bprint(bo, "dom=%s ip=%I\n", name, ad);
		break;
	case Thinfo:
		p = gmem(pp, vlen);
		Bprint(bo, "dom=%s hinfo=\"%s\"\n", name, p);
		break;
	case Ttxt:
		do{
			n = g8(pp);
			p = gmem(pp, n);
			Bprint(bo, "dom=%s txtrr=\"%.*s\"\n", name, n, p);
			free(p);
			p = nil;
			vlen -= n+1;
		}while(vlen > 0);
		break;
	case Trp:
		gname(pp, buf, s);
		if((p = strchr(s, '.')) != nil)
			*p = '@';
		p = nil;		/* don't free p */
		Bprint(bo, "dom=%s contact=%s\n", name, s);
		break;
	case Tns:
		gname(pp, buf, s);
		Bprint(bo, "dom=%s ns=%s\n", name, s);
		break;
	case Tptr:
		gname(pp, buf, s);
		Bprint(bo, "dom=%s ptr=%s\n", s, name);
		break;
	case Tcname:
		gname(pp, buf, s);
		Bprint(bo, "dom=%s cname=%s\n", name, s);
		break;
	case Tmx:
		n = g16(pp);
		gname(pp, buf, s);
		Bprint(bo, "dom=%s mx=%s pref=%ud\n", name, s, n);
		break;
	case Tsoa:
		Bprint(bo, "dom=%s soa= ", name);
		gname(pp, buf, s);
		Bprint(bo, "ns=%s ", s);
		gname(pp, buf, s);
		if((p = strchr(s, '.')) != nil)
			*p = '@';
		p = nil;		/* don't free p */
		Bprint(bo, "mbox=%s ", s);
	 	Bprint(bo, "serial=%ud ", g32(pp));
	 	Bprint(bo, "refresh=%ud ", g32(pp));
	 	Bprint(bo, "retry=%ud ", g32(pp));
	 	Bprint(bo, "expire=%ud ", g32(pp));
	 	Bprint(bo, "min=%ud\n", g32(pp));
		break;
	case Tsrv:
		i = g16(pp);
		j = g16(pp);
		k = g16(pp);
		gname(pp, buf, s);

		if((n = gettokens(name, a, 3, "._")) != 3)
			sysfatal("%s, %d fields - corrupt srv record\n", name, n);
		Bprint(bo, "srv=%s ", a[2]);
		Bprint(bo, "service=%s ", a[0]);
	 	Bprint(bo, "port=%ud ", k);
		Bprint(bo, "proto=%s ", a[1]);
	 	Bprint(bo, "priority=%ud ", i);
	 	Bprint(bo, "weight=%ud ", j);
		Bprint(bo, "dom=%s\n", s);
		break;
	case Tgpos:
		Bprint(bo, "# dom=%s ", name);
		Bprint(bo, "longitude=%d ", g16(pp));
		Bprint(bo, "latitude=%d ", g16(pp));
		Bprint(bo, "altitude=%d\n", g16(pp));
		break;
	case Tx25:
		p = gmem(pp, vlen);
		Bprint(bo, "dom=%s x25=%s\n", name, p);
		break;
	case Tisdn:
		p = gmem(pp, vlen);
		Bprint(bo, "dom=%s isdn=%s\n", name, p);
		break;
	case Twins:
		Bprint(bo, "wins=%s ", name);
		Bprint(bo, "replication=%s ", g32(pp)? "none": "active");
		Bprint(bo, "timeout=%d ", g32(pp));
		Bprint(bo, "cache=%d ", g32(pp));
		n = g32(pp);
		while(n--){
			ad = gmem(pp, 4);
			Bprint(bo, "server=%V ", ad);
			free(ad);
			ad = nil;
		}
		Bprint(bo, "\n");
		break;
	case Twinsr:
		Bprint(bo, "winsr=%s ", name);
		Bprint(bo, "replication=%s ", g32(pp)? "none": "active");
		Bprint(bo, "timeout=%d ", g32(pp));
		Bprint(bo, "cache=%d ", g32(pp));
		gname(pp, buf, s);
		Bprint(bo, "domain=%s\n", s);
		break;
	default:
		if(type > 0 && type < nelem(Typestr) && Typestr[type])
			Bprint(bo, "# %s=%s unsupported\n",
				Typestr[type], name);
		else{
			Bprint(bo, "# %ux=%s unknown; vlen %d\n",
				type, name, vlen);
//			xd(*pp, vlen);
		}
		skip(pp, vlen);
		break;
	}
	free(p);
	free(ad);
}

/*
 * Evade DNS poisioning, detect records that refer
 * to domains other than the one we asked about.
 */
int
mydom(char *me, char *dom)
{
	int m, d;

	m = strlen(me);
	d = strlen(dom);
	if (m > d)
		return 0;
	return cistrcmp(me, dom+(d-m)) == 0;
}

/*
 * issue axfr query
 */
void
query(int fd, char *mydomnm, ushort id)
{
	int len;
	uchar *p;
	uchar obuf[0xff];

	p = obuf;
	p16(&p, 0);		/* length, filled in later */
	p16(&p, id);		/* ID */
	p16(&p, 0);		/* flags */
	p16(&p, 1);		/* # questions */
	p16(&p, 0);		/* # answers */
	p16(&p, 0);		/* # authorities */
	p16(&p, 0);		/* # additional */
        pname(&p, mydomnm);	/* question name */
	p16(&p, Taxfr);		/* question type (AXFR - zone transfer) */
	p16(&p, Cin);		/* question class (Internet) */

	len = p - obuf;

	p = obuf;
	p16(&p, len - 2);	/* -2 as length is exclusive */

	if(write(fd, obuf, len) != len)
		sysfatal("write failed: %r");
}

Scan
reply(int fd)
{
	Scan sc;

	assert(sc.ptr = sc.base = malloc(2));
	alarm(5000);
	if(readn(fd, sc.base, 2) != 2)
		sysfatal("timeout reading length: %r");

	sc.len = g16(&sc.ptr);
	assert(sc.len >= 0);
	assert(sc.base = realloc(sc.base, 2 + sc.len + 1));
	sc.ptr = sc.base + 2;		/* recompute after realloc */
	if(readn(fd, sc.ptr, sc.len) != sc.len)
		sysfatal("timeout reading %d bytes: %r", sc.len);
	alarm(0);
	sc.ptr[sc.len] = '\0';
	if (debug)
		fprint(2, "%s: read %d bytes\n", argv0, sc.len);
	return sc;
}

/*
 * parse message header
 */
void
prsmsghdr(Scan *sc, Msghdr *msghdr, ushort id)
{
	int err;
	ushort mid;

	memset(msghdr, 0, sizeof *msghdr);
	mid = g16(&sc->ptr);
	if(mid != id)
		sysfatal("bad packet ID (sent %d got %d)", id, mid);
	err = g16(&sc->ptr) & 7;	/* flags */
	msghdr->nques = g16(&sc->ptr);		/* # questions */
	msghdr->nans = g16(&sc->ptr);		/* # answers */
	msghdr->naut = g16(&sc->ptr);		/* # authorities */
	msghdr->nadd = g16(&sc->ptr);		/* # additional */

	switch(err){
	case 0:
		break;	/* success */
	case 1:	sysfatal("bad request");
	case 2:	sysfatal("internal server failure");
	case 3:	sysfatal("resource does not exist");
	case 4:	sysfatal("request not supported");
	case 5:	sysfatal("permission denied");
	default:
		sysfatal("%d - unknown server error", err);
	}
	if(debug){
		fprint(2, "got ques %d ans %d auth %d add %d\n",
			msghdr->nques, msghdr->nans, msghdr->naut, msghdr->nadd);
		xd(sc->base, sc->len);
	}
}

/*
 * parse optional questions
 */
void
prsques(Scan *sc, int nques)
{
	int type, class;
	char name[Fqdnmax + 1];

	name[0] = '\0';
	type = Tsoa;
	class = Cin;
	while (nques-- > 0) {
		gname(&sc->ptr, sc->base, name);
		type = g16(&sc->ptr);
		class = g16(&sc->ptr);
	}
	if(debug){
		fprint(2, "got quest(s): nques %d type %d class %d name %s\n",
			nques, type, class, name);
		xd(sc->base, sc->len);
	}
}

/*
 * parse an answer (rr)
 */
int
prsansw(Scan *sc, char *mydomnm)
{
	int ok, rdlen, type, class;
	char name[Fqdnmax +1];
	static int nsoa;

	ok = 1;
	gname(&sc->ptr, sc->base, name);
	type = g16(&sc->ptr);
	class = g16(&sc->ptr);
	g32(&sc->ptr);			/* ignore TTL */
	rdlen = g16(&sc->ptr);		/* rdata length */

	if (rdlen > 512)
		fprint(2, "%s: rdlen %d > 512\n", argv0, rdlen);
	else if (rdlen <= 0 || rdlen > sc->len)
		fprint(2, "%s: crazy rdlen %d len %d\n", argv0, rdlen, sc->len);
	else if (class != Cin)
		fprint(2, "%s: %s: type %d class %d not 1; rdlen %d\n",
			argv0, name, type, class, rdlen);
	else if(type == Tsoa && nsoa++ > 0)
		ok = 0;			/* all done */
	else {
		if (!mydom(mydomnm, name))
			Bprint(bo, "#poison: %s: ", name);
		if(debug > 1)
			Bprint(bo, "type=%d len=%d name=%s ",
				type, sc->len, name);
		pr(&sc->ptr, sc->base, rdlen, name, type);
		Bflush(bo);			/* while debugging */
	}
	return ok;
}

void
main(int argc, char *argv[])
{
	int i, fd;
	ushort id;
	char net[64];
	Biobuf bout;
	Msghdr msghdr;
	Scan sc;

	bo = &bout;
	Binit(bo, 1, OWRITE);

	debug = 0;
	sc.base = sc.ptr = nil;
	sc.len = 0;
	setnetmtpt(net, sizeof net, nil);

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'x':
		setnetmtpt(net, sizeof(net), EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND;
	if(argc != 2)
		usage();

	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);

	if((fd = dial(netmkaddr(argv[0], "tcp", "dns"), 0, 0, 0)) < 0)
		sysfatal("%s can't dial - %r", argv[0]);

	id = time(nil) + getpid();
	query(fd, argv[1], id);

	/*
	 * process all messages in zone in reply
	 */
	notify(ding);
	for (;;) {
		sc = reply(fd);
		prsmsghdr(&sc, &msghdr, id);
		prsques(&sc, msghdr.nques);
		for (i = 0; i < msghdr.nans && sc.ptr < sc.base + 2 + sc.len;
		     i++)
			if (!prsansw(&sc, argv[1]))
				exits(0);		/* saw 2nd soa rr */
		/*
		 * ignore ns and ar records.
		 */
		if(debug && sc.ptr - sc.base > sc.len + 2)
			fprint(2, "skipped %ld bytes at end of packet\n",
				(sc.ptr - sc.base) - (sc.len + 2));
		free(sc.base);
		sc.ptr = sc.base = nil;
	}
}
