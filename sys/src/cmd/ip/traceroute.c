#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <ndb.h>

enum{
	Maxstring=	128,
	Maxpath=	256,
};

typedef struct DS DS;
struct DS {
	/* dist string */
	char	buf[Maxstring];
	char	*netdir;
	char	*proto;
	char	*rem;
};

typedef struct Icmp Icmp;
struct Icmp
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	ipcksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
};

enum
{			/* Packet Types */
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	EchoRequest	= 8,
	TimeExceed	= 11,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,

	ICMP_IPSIZE	= 20,
	ICMP_HDRSIZE	= 8,
};

char *argv0;
int debug;

void
usage(void)
{
	fprint(2, "usage: %s [-n] [protocol!]destination\n", argv0);
	exits("usage");
}

static int
csquery(DS *ds, char *clone, char *dest)
{
	int n, fd;
	char *p, buf[Maxstring];

	/*
	 *  open connection server
	 */
	snprint(buf, sizeof(buf), "%s/cs", ds->netdir);
	fd = open(buf, ORDWR);
	if(fd < 0){
		if(!isdigit(*dest)){
			werrstr("can't translate");
			return -1;
		}

		/* no connection server, don't translate */
		snprint(clone, sizeof(clone), "%s/%s/clone", ds->netdir, ds->proto);
		strcpy(dest, ds->rem);
		return 0;
	}

	/*
	 *  ask connection server to translate
	 */
	sprint(buf, "%s!%s", ds->proto, ds->rem);
	if(write(fd, buf, strlen(buf)) < 0){
		close(fd);
		return -1;
	}

	/*
	 *  get an address.
	 */
	seek(fd, 0, 0);
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if(n <= 0){
		werrstr("problem with cs");
		return -1;
	}

	buf[n] = 0;
	p = strchr(buf, ' ');
	if(p == 0){
		werrstr("problem with cs");
		return -1;
	}

	*p++ = 0;
	strcpy(clone, buf);
	strcpy(dest, p);
	return 0;
}

/*
 *  call the dns process and have it try to resolve the mx request
 */
static int
dodnsquery(DS *ds, char *ip, char *dom)
{
	char *p;
	Ndbtuple *t, *nt;

	p = strchr(ip, '!');
	if(p)
		*p = 0;

	t = dnsquery(ds->netdir, ip, "ptr");
	for(nt = t; nt != nil; nt = nt->entry)
		if(strcmp(nt->attr, "dom") == 0){
			strcpy(dom, nt->val);
			ndbfree(t);
			return 0;
		}
	ndbfree(t);
	return -1;
}

/*  for connection oriented protocols (il, tcp) we just need
 *  to try dialing.  resending is up to it.
 */
static int
tcpilprobe(int cfd, int dfd, char *dest, int interval)
{
	int n;
	char msg[Maxstring];

	USED(dfd);

	n = snprint(msg, sizeof msg, "connect %s", dest);
	alarm(interval);
	n = write(cfd, msg, n);
	alarm(0);
	return n;
}

/*
 *  for udp, we keep sending to an improbable port
 *  till we timeout or someone complains
 */
static int
udpprobe(int cfd, int dfd, char *dest, int interval)
{
	int n, i, rv;
	char msg[Maxstring];
	char err[Maxstring];

	seek(cfd, 0, 0);
	n = snprint(msg, sizeof msg, "connect %s", dest);
	if(write(cfd, msg, n)< 0)
		return -1;

	rv = -1;
	for(i = 0; i < 3; i++){
		alarm(interval/3);
		if(write(dfd, "boo hoo ", 8) < 0)
			break;
		/*
		 *  a hangup due to an error looks like 3 eofs followed
		 *  by a real error.  this is a qio.c qbread() strangeness
		 *  done for pipes.
		 */
		do {
			n = read(dfd, msg, sizeof(msg)-1);
		} while(n == 0);
		alarm(0);
		if(n > 0){
			rv = 0;
			break;
		}
		errstr(err, sizeof err);
		if(strstr(err, "alarm") == 0){
			werrstr(err);
			break;
		}
		werrstr(err);
	}
	alarm(0);
	return rv;
}

#define MSG "traceroute probe"
#define MAGIC 0xdead

static int
icmpprobe(int cfd, int dfd, char *dest, int interval)
{
	int x, i, n, len, rv;
	char buf[512];
	Icmp *ip;
	char msg[Maxstring];
	char err[Maxstring];

	seek(cfd, 0, 0);
	n = snprint(msg, sizeof msg, "connect %s", dest);
	if(write(cfd, msg, n)< 0)
		return -1;

	rv = -1;
	ip = (Icmp*)buf;
	for(i = 0; i < 3; i++){
		alarm(interval/3);
		ip->type = EchoRequest;
		ip->code = 0;
		strcpy((char*)ip->data, MSG);
		ip->seq[0] = MAGIC;
		ip->seq[1] = MAGIC>>8;
		len = ICMP_IPSIZE+ICMP_HDRSIZE+sizeof(MSG);
	
		/* send a request */
		if(write(dfd, buf, len) < len)
			break;
	
		/* wait for reply */
		n = read(dfd, buf, sizeof(buf));
		alarm(0);
		if(n < 0){
			errstr(err, sizeof err);
			if(strstr(err, "alarm") == 0){
				werrstr(err);
				break;
			}
			werrstr(err);
			continue;
		}
		x = (ip->seq[1]<<8)|ip->seq[0];
		if(n >= len)
		if(ip->type == EchoReply)
		if(x == MAGIC)
		if(strcmp((char*)ip->data, MSG) == 0){
			rv = 0;
			break;
		}
	}
	alarm(0);
	return rv;
}

static void
catch(void *a, char *msg)
{
	USED(a);
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

static int
call(DS *ds, char *clone, char *dest, int ttl, long *interval)
{
	int cfd, dfd, rv, n;
	char msg[Maxstring];
	char file[Maxstring];
	vlong start;

	notify(catch);

	/* start timing */
	start = nsec()/1000;
	rv = -1;

	cfd = open(clone, ORDWR);
	if(cfd < 0){
		werrstr("%s: %r", clone);
		return -1;
	}
	dfd = -1;

	/* get conversation number */
	n = read(cfd, msg, sizeof(msg)-1);
	if(n <= 0)
		goto out;
	msg[n] = 0;

	/* open data file */
	sprint(file, "%s/%s/%s/data", ds->netdir, ds->proto, msg);
	dfd = open(file, ORDWR);
	if(dfd < 0)
		goto out;

	/* set ttl */
	if(ttl)
		fprint(cfd, "ttl %d", ttl);

	/* probe */
	if(strcmp(ds->proto, "udp") == 0)
		rv = udpprobe(cfd, dfd, dest, 3000);
	else if(strcmp(ds->proto, "icmp") == 0)
		rv = icmpprobe(cfd, dfd, dest, 3000);
	else	/* il and tcp */
		rv = tcpilprobe(cfd, dfd, dest, 3000);
out:
	/* turn off alarms */
	alarm(0);
	*interval = nsec()/1000 - start;
	close(cfd);
	close(dfd);
	return rv;
}

/*
 *  parse a dial string.  default netdir is /net.
 *  default proto is tcp.
 */
static void
dial_string_parse(char *str, DS *ds)
{
	char *p, *p2;

	strncpy(ds->buf, str, Maxstring);
	ds->buf[Maxstring-3] = 0;

	p = strchr(ds->buf, '!');
	if(p == 0) {
		ds->netdir = 0;
		ds->proto = "tcp";
		ds->rem = ds->buf;
	} else {
		if(*ds->buf != '/'){
			ds->netdir = 0;
			ds->proto = ds->buf;
		} else {
			for(p2 = p; *p2 != '/'; p2--)
				;
			*p2++ = 0;
			ds->netdir = ds->buf;
			ds->proto = p2;
		}
		*p = 0;
		ds->rem = p + 1;
	}
	if(strchr(ds->rem, '!') == 0)
		strcat(ds->rem, "!32767");	
}

void
main(int argc, char **argv)
{
	int i, j, done;
	DS ds;
	char clone[Maxpath], dest[Maxstring], hop[Maxstring], dom[Maxstring];
	char err[Maxstring];
	long t[10], lo, hi, sum, x;
	char *p;
	int tries, notranslate;
	char *net;

	tries = 3;
	notranslate = 0;
	net = "/net";
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'n':
		notranslate++;
		break;
	case 't':
		p = ARGF();
		if(p)
			tries = atoi(p);
		break;
	case 'x':
		net = ARGF();
		break;
	default:
		usage();
	}ARGEND;

	if(argc < 1)
		usage();

	dial_string_parse(argv[0], &ds);

	if(ds.netdir == 0)
		ds.netdir = net;
	if(csquery(&ds, clone, dest) < 0){
		fprint(2, "%s: %s: %r\n", argv0, argv[0]);
		exits(0);
	}
	print("trying %s/%s!%s\n\n", ds.netdir, ds.proto, dest);
	print("                    round trip times in µs\n");
	print("                     low      avg     high\n");
	print("                --------------------------\n");

	done = 0;
	for(i = 1; i < 30; i++){
		for(j = 0; j < 10; j++){
			if(call(&ds, clone, dest, i, &t[j]) >= 0){
				strcpy(hop, dest);
				done = 1;
				continue;
			}
			errstr(err, sizeof err);
			if(debug)
				print("%s\n", err);
			if(strstr(err, "refused")){
				strcpy(hop, dest);
				p = strchr(hop, '!');
				if(p)
					*p = 0;
				done = 1;
			} else if(strstr(err, "unreachable")){
				strcpy(hop, dest);
				p = strchr(hop, '!');
				if(p)
					*p = 0;
				done = 1;
			} else if(strncmp(err, "ttl exceeded at ", 16) == 0){
				strcpy(hop, err+16);
			} else {
				strcpy(hop, "*");
				break;
			}
		}
		if(strcmp(hop, "*") == 0){
			print("*\n");
			continue;
		}
		lo = 10000000;
		hi = 0;
		sum = 0;
		for(j = 0; j < tries; j++){
			x = t[j];
			sum += x;
			if(x < lo)
				lo = x;
			if(x > hi)
				hi = x;
		}
		if(notranslate == 1 || dodnsquery(&ds, hop, dom) < 0)
			dom[0] = 0;
		print("%15.15s %8ld %8ld %8ld %s\n", hop, lo, sum/tries, hi, dom);
		if(done)
			break;
	}

	exits(0);
}
