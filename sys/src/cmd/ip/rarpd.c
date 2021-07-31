#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include "/sys/src/9/port/arp.h"

typedef struct Rarp	Rarp;

struct Rarp
{
	uchar	edst[6];
	uchar	esrc[6];
	uchar	type[2];
	uchar	hrd[2];
	uchar	pro[2];
	uchar	hln;
	uchar	pln;
	uchar	op[2];
	uchar	sha[6];
	uchar	spa[4];
	uchar	tha[6];
	uchar	tpa[4];
};

uchar	myip[4];
uchar	myether[6];
char	rlog[] = "ipboot";
char	*device = "ether";
int	debug;
Ndb	*db;

void
error(char *s)
{
	syslog(1, rlog, "error %s: %r", s);
	exits(s);
}

void
main(int argc, char *argv[])
{
	int edata, ectl;
	uchar buf[2048];
	long n;
	Rarp *rp;
	char ebuf[Ndbvlen];
	Arpentry entry;
	int arp;
	Ipinfo info;

	ARGBEGIN{
	case 'e':
		device = ARGF();
		break;
	case 'd':
		debug = 1;
		break;
	}ARGEND
	USED(argc, argv);

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	db = ndbopen(0);
	if(db == 0)
		error("can't open the database");

	edata = dial(netmkaddr("0x8035", device, 0), 0, 0, &ectl);
	if(edata < 0)
		error("can't open ethernet");

	if(myipaddr(myip, "/net/udp") < 0)
		error("can't get my ip address");
	sprint(ebuf, "/net/%s", device);
	if(myetheraddr(myether, ebuf) < 0)
		error("can't get my ether address");

	if(bind("#a", "/net", MBEFORE) < 0)
		error("binding /net/arp");
	if((arp = open("/net/arp/data", ORDWR)) < 0)
		error("open /net/arp/data");

	switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
	case -1:
		error("fork");
	case 0:
		break;
	default:
		exits(0);
	}

	for(;;){
		n = read(edata, buf, sizeof(buf));
		if(n <= 0)
			error("reading");
		if(n < sizeof(Rarp)){
			syslog(debug, rlog, "bad packet size %d", n);
			continue;
		}
		rp = (Rarp*)buf;
		if(rp->op[0]!=0 && rp->op[1]!=3){
			syslog(debug, rlog, "bad op %d %d %E",
				rp->op[1], rp->op[0], rp->esrc);
			continue;
		}

		if(debug)
			syslog(debug, rlog, "rcv se %E si %I te %E ti %I",
				 rp->sha, rp->spa, rp->tha, rp->tpa);

		sprint(ebuf, "%E", rp->tha);
		if(ipinfo(db, ebuf , 0, 0, &info) < 0){
			syslog(debug, rlog, "client lookup failed: %s", ebuf);
			continue;
		}

		memmove(rp->sha, myether, sizeof(rp->sha));
		memmove(rp->spa, myip, sizeof(rp->spa));

		rp->op[0] = 0;
		rp->op[1] = 4;
		memmove(rp->edst, rp->esrc, sizeof(rp->edst));
		memmove(rp->tpa, info.ipaddr, sizeof(rp->tpa));

		if(debug)
			syslog(debug, rlog, "send se %E si %I te %E ti %I",
				 rp->sha, rp->spa, rp->tha, rp->tpa);

		if(write(edata, buf, 42) != 42)
			error("write failed");

		memmove(entry.etaddr, rp->esrc, sizeof(entry.etaddr));
		memmove(entry.ipaddr, rp->tpa, sizeof(entry.ipaddr));
		if(write(arp, &entry, sizeof(entry)) < 0)
			perror("write arp entry");
	}
}
