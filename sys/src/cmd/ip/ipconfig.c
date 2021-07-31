/*
 * ipconfig - configure the ip multiplexor and ip protocols 
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include "/sys/src/9/port/bootp.h"

struct Protocols
{
	char	*dev;
	char	*net;
}protocols[] = {
	"#Iudp",	"/net",
	"#Itcp",	"/net",
	"#Iil",		"/net",
	0,		0
};

int 	noarp;
int 	arpprom;
char 	*Bad = "ipconfig: Failed to start";
char	*device = "ether";
char	*mask;
int	mflag;
char	iplog[] = "ip";
uchar	eaddr[6];

#define WRITESTR(fd, X)	if(write(fd, X, sizeof(X)-1) < 0) \
		{ syslog(1, iplog, "write %s: %r", X); exits(Bad); }
#define ARPD		"/bin/ip/arpd"

int	lookupip(Ipinfo*);
int	bootp(Ipinfo *iip);
void	csaddnet(char*);

extern uchar classmask[4][4];

void
usage(void)
{
	fprint(2, "usage: ipconfig [-a] [-p] [-e device] [-m ipmask] [ipaddr] \n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, nobootp, secondary;
	int ectl, edata, fd;
	char buf[128];
	char maskbuf[32];
	Ipinfo ii;
	uchar ip[4];

	nobootp = 0;
	secondary = 0;
	ARGBEGIN{
	case 'a':
		noarp = 1;
		break;
	case 'p':
		arpprom = 1;
		break;
	case 'e':
		device = ARGF();
		break;
	case 's':
		secondary = 1;
		break;
	case 'm':
		mask = ARGF();
		mflag++;
		break;
	case 'b':
		nobootp++;
		break;
	default:
		usage();
	}ARGEND

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	edata = dial(netmkaddr("0x800", device, 0), 0, 0, &ectl);
	if(edata < 0){
		syslog(1, iplog, "open: ethernet control: %r");
		exits(Bad);
	}
	close(edata);

	switch(argc){
	default:
		usage();
		break;
	case 1:
		memset(&ii, 0, sizeof(ii));
		parseip(ii.ipaddr, argv[0]);
		memcpy(ii.ipmask, classmask[ii.ipaddr[0]>>6], 4);
		break;
	case 0:
		if(lookupip(&ii) < 0)
			nobootp = 0;
		break;
	}

	/* see if an ip address has already been set */
	if(!secondary && (myipaddr(ip, "#Itcp/tcp") < 0 || *ip == 0)){
		WRITESTR(ectl, "push arp");
		WRITESTR(ectl, "push internet");
		WRITESTR(ectl, "push permanent");

		for(i = 0; protocols[i].dev; i++) {
			if(bind(protocols[i].dev, protocols[i].net, MBEFORE) < 0)
				fprint(2, "ipconfig: bind(%s, %s) failed: %r\n", 
					protocols[i].dev, protocols[i].net);
		}

		if(nobootp == 0)
			bootp(&ii);

		if(mflag == 0){
			sprint(maskbuf, "%I", ii.ipmask);
			mask = maskbuf;
		}

		if(*ii.ipaddr == 0){
			fprint(2, "ipconfig: local IP address not found, IP not configured.\n");
			fprint(2, "\tTo use IP, edit /lib/ndb/local and reboot.\n");
			exits(Bad);
		}

		sprint(buf, "setip %I %s", ii.ipaddr, mask);
		print("ipconfig: %s\n", buf);
		if(write(ectl, buf, strlen(buf)) < 0) {
			syslog(1, iplog, "write: setip: %r");
			exits(Bad);
		}

		/* set default router */
		if(*ii.gwip){
			sprint(buf, "add 0.0.0.0 0.0.0.0 %I", ii.gwip);
			fd = open("#P/iproute", OWRITE);
			if(fd >= 0){
				print("ipconfig: %s\n", buf);
				write(fd, buf, strlen(buf));
				close(fd);
			}
		}
	} else if(secondary) {
		WRITESTR(ectl, "push arp");
		WRITESTR(ectl, "push internet");
		WRITESTR(ectl, "push permanent");

		if(mflag == 0){
			sprint(maskbuf, "%I", ii.ipmask);
			mask = maskbuf;
		}

		sprint(buf, "setip %I %s", ii.ipaddr, mask);
		print("ipconfig: %s\n", buf);
		if(write(ectl, buf, strlen(buf)) < 0) {
			syslog(1, iplog, "write: setip: %r");
			exits(Bad);
		}
	}
	close(ectl);
	sprint(buf, "%I", ii.ipaddr);
	putenv("ipaddr", buf);

	csaddnet("il");
	csaddnet("tcp");
	csaddnet("udp");
	if(noarp)
		exits(0);

	switch(fork()) {
	case 0:
		rfork(RFENVG|RFNAMEG|RFNOTEG);
		/* Turn into an arp server */
		if(arpprom)
			execl(ARPD, "arpd", "-pe", device, 0);
		else
			execl(ARPD, "arpd", "-e", device, 0);

		syslog(1, iplog, "ipconfig: exec arpd: %r");
		for(;;)
			sleep(10000000);
	case -1:
		syslog(1, iplog, "fork: %r");
		exits(Bad);
	default:
		exits(0);
	}
}

int
lookupip(Ipinfo *iip)
{
	char buf[64];
	char *sysname;
	char *ename;
	char *ipname;
	char ebuf[32];
	int rv;
	Ndb *db;

	/* find out any local info */
	ipname = getenv("ipaddr");
	if(ipname && *ipname == 0)
		ipname = 0;

	sysname = getenv("sysname");
	if(sysname && *sysname == 0)
		sysname = 0;

	sprint(buf, "/net/%s", device);
	if(myetheraddr(eaddr, buf) < 0)
		ename = 0;
	else {
		sprint(ebuf, "%E", eaddr);
		ename = ebuf;
	}

	if(ename == 0 && sysname ==0 && ipname == 0)
		return -1;
	db = ndbopen(0);
	if(db == 0)
		return -1;
	rv = ipinfo(db, ename, ipname, sysname, iip);
	ndbclose(db);

	return rv;
}

static int
catchint(void *a, char *note)
{
	USED(a);
	if(strcmp(note, "alarm") == 0)
		return 1;
	return 0;
}

int
bootp(Ipinfo *iip)
{
	int fd;
	int n;
	int tries;
	Bootp req, *rp;
	char *field[4];
	char buf[1600];

	/* open a udp connection for bootp and fill in a packet */
	fd = dial("udp!255.255.255.255!67", "68", 0, 0);
	if(fd < 0){
		fprint(2, "ipconfig: can't bootp '%r', hope that's OK\n");
		return -1;
	}
	memset(&req, 0, sizeof(req));
	req.op = Bootrequest;
	req.htype = 1;		/* ethernet */
	req.hlen = 6;		/* ethernet */
	memmove(req.chaddr, eaddr, sizeof(req.chaddr));
	memset(req.file, 0, sizeof(req.file));
	strcpy(req.vend, "p9  ");

	/* broadcast bootp's till we get a reply, or 3 times around the loop */
	atnotify(catchint, 1);
	tries = 0;
	field[0] = 0;
	for(rp = 0; rp == 0 && tries++ < 10;){
		alarm(1000);
		if(write(fd, &req, sizeof(req)) < 0){
			fprint(2, "ipconfig: can't write bootp '%r', hope that's OK\n");
			close(fd);
			return -1;
		}
		setfields(" \t");
		for(;;){
			rp = 0;
			memset(buf, 0, sizeof(buf));
			n = read(fd, buf, sizeof(buf));
			if(n <= 0)
				break;
			rp = (Bootp*)buf;
			field[0] = 0;;
			if(memcmp(req.chaddr, rp->chaddr, 6) == 0
			&& rp->htype == 1
			&& rp->hlen == 6
			&& strncmp(rp->vend, "p9  ", 4) == 0
			&& getfields(rp->vend+4, field, 4) == 4){
				memmove(iip->ipaddr, rp->yiaddr, sizeof(iip->ipaddr));
				parseip(iip->ipmask, field[0]);
				parseip(iip->auip, field[2]);
				parseip(iip->gwip, field[3]);
				maskip(iip->ipaddr, iip->ipmask, iip->ipnet);
				break;
			}
		}
		alarm(0);
	}
	close(fd);
	return 0;
}

void
csaddnet(char *name)
{
	int fd;
	char buf[NAMELEN+6];

	sprint(buf, "add %s", name);
	fd = open("/net/cs", OWRITE);
	if(fd < 0)
		return;
	write(fd, buf, strlen(buf));
	close(fd);
}
