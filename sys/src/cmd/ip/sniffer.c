#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include <ctype.h>

enum
{
	Nring	= 2*1024*1024,
	Nfilter	= 32,
};

int	debug;
int	pflag = 1;
uchar	ring[Nring];	/* circular byte buffer */
uchar	*rd;		/* first unprocessed byte */
uchar	*wr;		/* first free byte */
uchar	*wrap;		/* current wrap point */
uchar	*e;		/* top of ring */
int	stop;
int	timefd;
ulong	start;

typedef struct Filter Filter;
struct Filter
{
	uchar	val[12];
	int	len;
	int	pos;
};
Filter	fil[Nfilter];
int	nfil;

void	tostdout(void);
void	error(char *error, ...);
void	fromnet(int fd);
ulong	gettime(void);
void	setfilter(char*);
void	starttime(void);

void
main(int argc, char **argv)
{
	char dev[256];
	int fd, cfd, pid, ppid;
	char *p;

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'p':
		pflag = 0;
		break;
	case 'F':
		p = ARGF();
		if(p == 0)
			break;
		setfilter(p);
		break;
	}ARGEND;

	if(argc > 0)
		snprint(dev, sizeof(dev), "%s!-1", argv[0]);
	else
		strcpy(dev, "/net/ether0!-1");

	fd = dial(dev, 0, 0, &cfd);
	if(fd < 0)
		error("opening %s: %r", dev);
	if(pflag && fprint(cfd, "promiscuous") < 0)
		error("setting promiscuous mode");


	starttime();

	wr = rd = ring;
	wrap = e = ring+sizeof(ring);
	ppid = getpid();
	switch(pid = rfork(RFMEM|RFPROC)){
	case -1:
		error("can't fork: %r");
	case 0:
		tostdout();
		postnote(PNPROC, ppid, "die yankee pig dog");
		break;
	default:
		fromnet(fd);
		postnote(PNPROC, pid, "die yankee pig dog");
		break;
	}
	exits(0);
}

void
fromnet(int fd)
{
	int n;
	ulong now;
	Filter *fe, *fp;

	fe = &fil[nfil];
	while(stop == 0){
		if(rd > wr){
			if((n = rd - wr) < 2000){
				sleep(0);
				continue;
			}
		} else {
			if((n = e - wr) < 2000){
				wrap = wr;
				wr = ring;
				continue;
			}
		}
		n = read(fd, wr+6, n-6);
		if(n <= 0)
			break;
		if(nfil){
			for(fp = fil; fp < fe; fp++)
				if(memcmp(wr+6+fp->pos, fp->val, fp->len) == 0)
					break;
			if(fp == fe)
				continue;
		}
		now = gettime();
		wr[0] = n>>8;
		wr[1] = n;
		wr[2] = now>>24;
		wr[3] = now>>16;
		wr[4] = now>>8;
		wr[5] = now;
		wr += n+6;
	}
}

void
catch(void*, char*)
{
	stop = 1;
	noted(NCONT);
}

void
tostdout(void)
{
	int n;

	notify(catch);

	while(stop == 0){
		if(rd == wr){
			sleep(250);
			continue;
		}
		if(wr > rd)
			n = wr - rd;
		else
			n = wrap - rd;
		if(write(1, rd, n) < 0)
			break;
		rd += n;
		if(e - rd < 2000)
			rd = ring;
	}
}

void
error(char *error, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, error);
	vseprint(buf, &buf[sizeof(buf)], error, arg);
	va_end(arg);
	fprint(2, "sniffer: %s\n", buf);
	exits(buf);
}

void
starttime(void)
{
	timefd = open("/dev/cputime", OREAD);
	start = gettime();
}

ulong
gettime(void)
{
	char x[3*12+1];

	x[0] = 0;
	seek(timefd, 0, 0);
	read(timefd, x, 3*12);
	x[3*12] = 0;
	return strtoul(x+2*12, 0, 0)-start;
}

void
newfilter(uchar *v, int len, int off)
{
	if(nfil >= Nfilter)
		return;
	memmove(fil[nfil].val, v, len);
	fil[nfil].len = len;
	fil[nfil].pos = off;
	nfil++;
}

char*
lookup(char *sattr, char *sval, char *tattr, char *tval)
{
	static Ndb *db;
	char *attrs[1];
	Ndbtuple *t;

	if(db == nil)
		db = ndbopen(0);
	if(db == nil)
		return nil;

	if(sattr == nil)
		sattr = ipattr(sval);

	attrs[0] = tattr;
	t = ndbipinfo(db, sattr, sval, attrs, 1);
	if(t == nil)
		return nil;
	strcpy(tval, t->val);
	ndbfree(t);
	return tval;
}

void
setfilter(char *f)
{
	char *p;
	uchar s[2];
	uchar ea[6];
	uchar ipa[IPv4addrlen];
	char buf[Ndbvlen];
	int c, etheraddr, ipaddr, port, found;

	memset(ea, 0, sizeof(ea));
	memset(ipa, 0, sizeof(ipa));

	etheraddr = port = ipaddr = 1;
	if(strlen(f) != 12)
		etheraddr = 0;
	for(p = f; *p; p++){
		c = *p;
		if(isxdigit(c)){
			if(!isdigit(c))
				ipaddr = port = 0;
		} else if(c == '.')
			etheraddr = port = 0;
		else
			ipaddr = port = etheraddr = 0;
	}
	found = 0;
	if(etheraddr){
		/* assume ether address */
		parseether(ea, f);
		newfilter(ea, 6, 0);
		newfilter(ea, 6, 6);
		found = 1;
	} else if(port) {
		/* assume is tcp/udp port.  this comes before ip address
		 * because if port==1, ipaddress==1, but not vice versa.
		 */
		port = atoi(f);
		s[0] = port >> 8;
		s[1] = port & 0xff;
		/* proto is in 9 udp=17 tcp=6 ports are in 20, 22 */
		newfilter(s, 2, 14+20);
		newfilter(s, 2, 14+20+2);
		found = 1;
	} else if(ipaddr){
		/* assume ip address */
		v4parseip(ipa, f);
		newfilter(ipa, IPv4addrlen, 14+12);
		newfilter(ipa, IPv4addrlen, 14+12+IPv4addrlen);
		found = 1;
	} else {
		if(lookup(nil, f, "ether", buf) != nil){
			parseether(ea, buf);
			newfilter(ea, 6, 0);
			newfilter(ea, 6, 6);
			found = 1;
		} else if(lookup(nil, f, "ip", buf) != nil){
			v4parseip(ipa, buf);
			newfilter(ipa, IPv4addrlen, 14+12);
			newfilter(ipa, IPv4addrlen, 14+12+IPv4addrlen);
			found = 1;
		}
	}

	if(found == 0)
		error("%s's address unknown", f);
	else
		fprint(2, "sniffing %I %E\n", ipa, ea);
	fprint(2, "nfil = %d\n", nfil);
}
