/*
 * ip not on US gov't's list of bad guys?
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <ip.h>

enum {
	Dialwaitsecs = 60,		/* be patient */
	Readwaitsecs = 60,
	Maxstr = 100,			/* arbitrary limit */
};

int cfd = -1;
int verbose;

typedef struct Whois Whois;

char *badguys[] =
{
	"CU",		/* cuba */
	"IR",		/* iran */
	"KP",		/* north korea */
	"LY",		/* libya */
	"SD",		/* sudan */
	"SY",		/* syria */
};

struct Whois {
	uchar	ip[IPaddrlen];

	uchar	lo[IPaddrlen];
	uchar	hi[IPaddrlen];

	char	referral[Maxstr];
	char	country[Maxstr];
	char	netname[Maxstr];
	char	desc[Maxstr];
};

static char *
skipsp(char *p)
{
	while (isascii(*p) && isspace(*p))
		p++;
	return p;
}

static char *
skipip(char *p)
{
	while (isascii(*p) && (isxdigit(*p) || *p == '.' || *p == ':'))
		p++;
	return p;
}

static int
parsename(char *name, uchar *lo, uchar *hi)
{
	char *p;
	uchar mask[IPaddrlen];

	p = skipsp(name);
	if (parseip(lo, p) == -1)
		return -1;
	p = skipsp(skipip(p));
	if(*p == '-'){
		p = skipsp(p+1);
		if(parseip(hi, p) == -1)
			return -1;
		p = skipip(p);
	}else if(*p == '/'){		/* mask follows */
		parseipmask(mask, p);
		maskip(lo, mask, hi);
		while(++p, isascii(*p) && (isdigit(*p) || *p == '.'))
			;
	}else
		memmove(lo, hi, IPaddrlen);
	if(*skipsp(p) != '\0')
		return -1;
	return 0;
}

static void
bad(char *v, char *server, uchar *ipx)
{
	print("bad malformed ip range %s from %s for %I\n", v, server, ipx);
	exits(0);
}

static int
saveval(Whois *w, char *server, char *v, char *k, uchar *lo, uchar *hi,
	uchar *ipx)
{
	char *q, *next;
	static uchar zip[IPaddrlen];

	if(cistrcmp(k, "country") == 0 || cistrcmp(k, "country-code") == 0)
		strcpy(w->country, v);
	else if(cistrcmp(k, "netname") == 0 || cistrcmp(k, "network-name") == 0)
		strcpy(w->netname, v);
	else if((cistrcmp(k, "descr") == 0 || cistrcmp(k, "orgname") == 0
	|| cistrcmp(k, "org-name") == 0 || cistrcmp(k, "owner") == 0
	|| cistrcmp(k, "organization") == 0))
		strcpy(w->desc, v);
	else if(cistrcmp(k, "referralserver") == 0)
		strcpy(w->referral, v);
	else if((cistrcmp(k, "netrange") == 0 || cistrcmp(k, "cidr") == 0
	|| cistrcmp(k, "inetnum") == 0 || cistrcmp(k, "ip-network") == 0)
	&& ipcmp(w->lo, zip) == 0 && ipcmp(w->hi, zip) == 0){
		for(q = v; q && *q; q = next){
			next = strchr(q, ',');
			if(next != nil)
				*next++ = 0;
			if (parsename(q, lo, hi)== -1)
				bad(v, server, ipx);
			if(ipcmp(lo, w->ip) <= 0 && ipcmp(w->ip, hi) <= 0){
				memmove(w->lo, lo, IPaddrlen);
				memmove(w->hi, hi, IPaddrlen);
				return 0;
			}
		}
		return -1;
	}else
		return -1;
	return 0;
}

int
whois1(Whois *w, char *server)
{
	int fd;
	char *addr, *k, *p;
	uchar ip[IPaddrlen], ipx[IPaddrlen], lo[IPaddrlen], hi[IPaddrlen];
	Biobuf b;

	memmove(ipx, w->ip, IPaddrlen);
	addr = netmkaddr(server, "tcp", "whois");
	if(verbose)
		fprint(2, "## dial %s for %I\n", addr, ipx);
	alarm(Dialwaitsecs * 1000);
	fd = dial(addr, nil, nil, nil);
	alarm(0);
	if(fd < 0){
		print("bad dial %s: %r\n", addr);
		return -1;
	}

	Binit(&b, fd, OREAD);
	memmove(ip, w->ip, IPaddrlen);

	/* arin query syntax changed 26 June 2010 */
	fprint(fd, "%s%I\n", strstr(server, "arin.") != nil? "+ n ": "", ip);

	w->country[0] = 0;
	memset(w->lo, 0, IPaddrlen);
	memset(w->hi, 0, IPaddrlen);
	w->referral[0] = 0;
	alarm(Readwaitsecs * 1000);
	while((p = Brdline(&b, '\n')) != nil){
		p[Blinelen(&b)-1] = 0;
		if(Blinelen(&b) >= 2 && p[Blinelen(&b)-2] == '\r')
			p[Blinelen(&b)-2] = 0;
		if(verbose)
			fprint(2, "# %s\n", p);

		/* brazil's whois server is non-standard */
		if(strstr(p, "Copyright registro.br") != nil ||
		    strstr(p, "Copyright (c) Nic.br") != nil ||
		    strstr(p, "//registro.br/") != nil){
			strcpy(w->country, "BR");
			continue;
		}

		k = p;
		if(cistrncmp(p, "network:", 8) == 0)
			k += 8;
		if((p = strchr(k, ':')) == nil)
			continue;
		*p++ = 0;
		p = skipsp(p);
		if(strlen(p) >= Maxstr)
			p[Maxstr-1] = 0;
		saveval(w, server, p, k, lo, hi, ipx);
	}
	alarm(0);
	return 0;
}

void
whois(uchar *ip)
{
	char server[Maxstr], *p, *status;
	uchar ipx[IPaddrlen], lo[IPaddrlen], hi[IPaddrlen], mask[IPaddrlen];
	Whois w;
	int i, nrefer;
	static char lastcountry[Maxstr];

	/* follow a referrer chain to the end */
	nrefer = 0;
	memmove(ipx, ip, IPaddrlen);
	strcpy(server, "whois.arin.net");
	lastcountry[0] = 0;
	for (;;) {
		memset(&w, 0, sizeof w);
		memmove(w.ip, ip, IPaddrlen);
		if(whois1(&w, server) < 0 ||
		    ipcmp(w.lo, ip) > 0 || ipcmp(w.hi, ip) < 0){
			print("bad useless response from %s for %I\n",
				server, ipx);
			print("w.lo %I ip %I w.hi %I\n", w.lo, ip, w.hi); // TODO DEBUG
			if(isv4(ipx))
				memmove(mask, defmask(ipx-IPv4off), IPaddrlen);
			else
				memset(mask, ~0, IPaddrlen);
			maskip(w.ip, mask, w.hi);
			return;
		}
		if(w.referral[0] == '\0' || nrefer++ >= 10)
			break;
		p = w.referral;
		if(strncmp(p, "whois://", 8) == 0)
			p += 8;
		if(strncmp(p, "rwhois://", 9) == 0){
			/*
			 * Only remember the country doing the referring
			 * for rwhois://, which seems to be used to
			 * defer to local ISPs.
			 */
			/* in fact, who cares about rwhois anyway? */
			break;
//			strcpy(lastcountry, w.country);
//			p += 9;
		}
		strcpy(server, p);
		p = strchr(server, ':');
		if(p)
			*p = 0;
	}

	/* no referrer or referrer chain is too deep */
	memmove(lo, w.lo, IPaddrlen);
	memmove(hi, w.hi, IPaddrlen);
	status = "ok";
	if(w.country[0] == 0)
		strcpy(w.country, lastcountry);
	if(w.country[0] == 0)
		status = "bad";
	else {
		for(i = 0; i < nelem(badguys); i++)
			if(cistrcmp(w.country, badguys[i]) == 0) {
				status = "bad";
				break;
			}
		if(cfd >= 0){
			seek(cfd, 0, 2);
			fprint(cfd, "%s %I-%I %q %q %q\n", status, lo, hi,
				w.country, w.netname, w.desc);
		}
	}
	print("%s %I-%I %q %q %q\n", status, lo, hi, w.country, w.netname,
		w.desc);
}

void
cwhois(uchar *ip)
{
	Biobuf b;
	char *p, *q, *r;
	uchar lo[IPaddrlen], hi[IPaddrlen];

	if((cfd = open("/sys/lib/ipok.cache", ORDWR)) < 0)
		return;
	Binit(&b, cfd, OREAD);
	while((p = Brdline(&b, '\n')) != nil){
		p[Blinelen(&b)-1] = 0;
		q = strchr(p, ' ');
		if(q == nil)
			continue;
		q++;
		r = strchr(q, ' ');
		if(r)
			*r = 0;
		if(parsename(q, lo, hi) < 0)
			continue;
		if(r)
			*r = ' ';
		if(ipcmp(lo, ip) <= 0 && ipcmp(ip, hi) <= 0){
			print("%s (cached)\n", p);
			exits(0);
		}
	}
	Bterm(&b);
}

void
usage(void)
{
	fprint(2, "usage: aux/ipok ip...\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	uchar ip[IPaddrlen];

	ARGBEGIN{
	case 'v':
		verbose++;
		break;
	default:
		usage();
	}ARGEND

	if(argc < 1)
		usage();

	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	quotefmtinstall();

	for(i=0; i<argc; i++){
		if(fork() == 0){
			parseip(ip, argv[0]);
			cwhois(ip);
			whois(ip);
			exits(0);
		}
	}
	while(waitpid() > 0)
		;
	exits(0);
}
