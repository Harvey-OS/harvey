#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "kernel.h"
#include "ip.h"

static	ulong	fsip;
static	ulong	auip;
static	ulong	gwip;
static	ulong	ipmask;
static	ulong	ipaddr;

uchar	sys[NAMELEN];

enum
{
	Bootrequest = 1,
	Bootreply   = 2,
};

typedef struct Bootp
{
	uchar	op;		/* opcode */
	uchar	htype;		/* hardware type */
	uchar	hlen;		/* hardware address len */
	uchar	hops;		/* hops */
	uchar	xid[4];		/* a random number */
	uchar	secs[2];	/* elapsed snce client started booting */
	uchar	pad[2];
	uchar	ciaddr[4];	/* client IP address (client tells server) */
	uchar	yiaddr[4];	/* client IP address (server tells client) */
	uchar	siaddr[4];	/* server IP address */
	uchar	giaddr[4];	/* gateway IP address */
	uchar	chaddr[16];	/* client hardware address */
	uchar	sname[64];	/* server host name (optional) */
	uchar	file[128];	/* boot file name */
	uchar	vend[128];	/* vendor-specific goo */
} Bootp;

/*
 * bootp returns:
 *
 * "fsip d.d.d.d
 * auip d.d.d.d
 * gwip d.d.d.d
 * ipmask d.d.d.d
 * ipaddr d.d.d.d"
 *
 * where d.d.d.d is the IP address in dotted decimal notation, and each
 * address is followed by a newline.
 */
enum
{
	bootpreadlen = sizeof("fsip") + sizeof("auip") + sizeof("gwip")
		+ sizeof("ipmask") + sizeof("ipaddr")
		+ 5 * sizeof("000.000.000.000") + sizeof("")
};

static	Bootp	req;
static	int	recv;
static	int	done;
static	Rendez	bootpr;
static	char	rcvbuf[512];


/*
 * Parse the vendor specific fields according to RFC 1084.
 * We are overloading the "cookie server" to be the Inferno 
 * authentication server and the "resource location server"
 * to be the Inferno file server.
 *
 * If the vendor specific field is formatted properly, it
 * will being with the four bytes 99.130.83.99 and end with
 * an 0xFF byte.
 */
static void
parsevend(uchar* vend)
{
	/* The field must start with 99.130.83.99 to be compliant */
	if ((vend[0] != 99) || (vend[1] != 130) ||
	    (vend[2] != 83) || (vend[3] != 99))
		return;

	/* Skip over the magic cookie */
	vend += 4;

	while ((vend[0] != 0) && (vend[0] != 0xFF)) {
		switch (vend[0]) {
		case 1:	/* Subnet mask field */
			/* There must be only on subnet mask */
			if (vend[1] != 4)
				return;

			ipmask = (vend[2]<<24)|
				 (vend[3]<<16)|
				 (vend[4]<<8)|
				  vend[5];
			break;

		case 3:	/* Gateway/router field */
			/* We are only concerned with first address */
			if (vend[1] < 4)
				return;

			gwip =	(vend[2]<<24)|
				(vend[3]<<16)|
				(vend[4]<<8)|
				 vend[5];
			break;

		case 8:	/* "Cookie server" (auth server) field */
			/* We are only concerned with first address */
			if (vend[1] < 4)
				return;

			auip =	(vend[2]<<24)|
				(vend[3]<<16)|
				(vend[4]<<8)|
				 vend[5];
			break;

		case 11:	/* "Resource loc server" (file server) field */
			/* We are only concerned with first address */
			if (vend[1] < 4)
				return;

			fsip =	(vend[2]<<24)|
				(vend[3]<<16)|
				(vend[4]<<8)|
				 vend[5];
			break;

		default:	/* Everything else stops us */
			return;
		}

		/* Skip over the field */
		vend += vend[1] + 2;
	}
}

void
rcvbootp(void *a)
{
	int n, fd;
	Bootp *rp;

	fd = (int)a;
	while(done == 0) {
		n = kread(fd, rcvbuf, sizeof(rcvbuf));
		if(n <= 0)
			break;
		rp = (Bootp*)rcvbuf;
		if (memcmp(req.chaddr, rp->chaddr, 6) == 0 &&
		   rp->htype == 1 && rp->hlen == 6) {
			ipaddr = (rp->yiaddr[0]<<24)|
				 (rp->yiaddr[1]<<16)|
				 (rp->yiaddr[2]<<8)|
				  rp->yiaddr[3];
			parsevend(rp->vend);
			break;
		}
	}

	recv = 1;
	wakeup(&bootpr);
	pexit("", 0);
}

char*
bootp(Ipifc *ifc)
{
	int fd, tries, n;
	char ia[5+3*16], im[16], *av[3];
	char nipaddr[4], ngwip[4], nipmask[4];

	av[1] = "0.0.0.0";
	av[2] = "0.0.0.0";
	ipifcadd(ifc, av, 3);

	fd = kdial("udp!255.255.255.255!67", "68", nil, nil);
	if(fd < 0)
		return "bootp dial failed";

	/* create request */
	memset(&req, 0, sizeof(req));
	req.op = Bootrequest;
	req.htype = 1;			/* ethernet (all we know) */
	req.hlen = 6;			/* ethernet (all we know) */

	/* Hardware MAC address */
	memmove(req.chaddr, ifc->mac, 6);
	/* Fill in the local IP address if we know it */
	ipv4local(ifc, req.ciaddr);
	memset(req.file, 0, sizeof(req.file));
	strcpy((char*)req.vend, "p9  ");

	kproc("rcvbootp", rcvbootp, (void*)fd);

	/*
	 * broadcast bootp's till we get a reply,
	 * or fixed number of tries
	 */
	tries = 0;
	while(recv == 0) {
		if(kwrite(fd, &req, sizeof(req)) < 0)
			print("bootp: write: %r");

		tsleep(&bootpr, return0, 0, 1000);
		if(++tries > 10) {
			print("bootp: timed out\n");
			break;
		}
	}
	kclose(fd);
	done = 1;

	av[1] = "0.0.0.0";
	av[2] = "0.0.0.0";
	ipifcrem(ifc, av, 3, 1);

	hnputl(nipaddr, ipaddr);
	sprint(ia, "%V", nipaddr);
	hnputl(nipmask, ipmask);
	sprint(im, "%V", nipmask);
	av[1] = ia;
	av[2] = im;
	ipifcadd(ifc, av, 3);

	if(gwip != 0) {
		hnputl(ngwip, gwip);
		n = sprint(ia, "add 0.0.0.0 0.0.0.0 %V", ngwip);
		routewrite(ifc->conv->p->f, nil, ia, n);
	}
	return nil;
}

int
bootpread(char *bp, ulong offset, int len)
{
	int n;
	char buf[bootpreadlen], a[4];

	hnputl(a, fsip);
	n = sprint(buf, "fsip %15V\n", a);
	hnputl(a, auip);
	n += sprint(buf + n, "auip %15V\n", a);
	hnputl(a, gwip);
	n += sprint(buf + n, "gwip %15V\n", a);
	hnputl(a, ipmask);
	n += sprint(buf + n, "ipmask %15V\n", a);
	hnputl(a, ipaddr);
	sprint(buf + n, "ipaddr %15V\n", a);

	return readstr(offset, bp, len, buf);
}
