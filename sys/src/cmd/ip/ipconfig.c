/*
 * ipconfig - configure the ip multiplexor and ip protocols 
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

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

#define WRITESTR(fd, X)	if(write(fd, X, sizeof(X)-1) < 0) \
		{ syslog(1, iplog, "write %s: %r", X); exits(Bad); }
#define ARPD		"/bin/ip/arpd"

int	lookupip(Ipinfo*);

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
	int i;
	char buf[128];
	int ectl, edata, fd;
	char maskbuf[32];
	Ipinfo ii;
	uchar ip[4];

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
	case 'm':
		mask = ARGF();
		mflag++;
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
		if(lookupip(&ii) < 0){
			syslog(1, iplog, "lookup: my ip address");
			exits(Bad);
		}
	}

	sprint(buf, "%I", ii.ipaddr);
	putenv("ipaddr", buf);
	if(mflag == 0){
		sprint(maskbuf, "%I", ii.ipmask);
		mask = maskbuf;
	}

	/* see if an ip address has already been set */
	if(myipaddr(ip, "#Itcp/tcp") < 0 || *ip == 0){
		WRITESTR(ectl, "push arp");
		WRITESTR(ectl, "push internet");
	
		sprint(buf, "setip %I %s", ii.ipaddr, mask);
		print("ipconfig: %s\n", buf);
		if(write(ectl, buf, strlen(buf)) < 0) {
			syslog(1, iplog, "write: setip: %r");
			exits(Bad);
		}
	
		WRITESTR(ectl, "push tcp");
	
		for(i = 0; protocols[i].dev; i++) {
			if(bind(protocols[i].dev, protocols[i].net, MBEFORE) < 0)
				syslog(1, iplog, "ipconfig: bind(%s, %s) failed: %r", 
					protocols[i].dev, protocols[i].net);
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
	}
	close(ectl);

	if(noarp)
		exits(0);

	switch(fork()) {
	case 0:
		rfork(RFENVG|RFNAMEG|RFNOTEG);
		/* Turn into an arp server */
		if(arpprom)
			execl(ARPD, "arpd", "-p", 0);
		else
			execl(ARPD, "arpd", 0);

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
	uchar eaddr[6];
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
