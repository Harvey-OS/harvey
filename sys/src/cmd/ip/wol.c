/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* send wake-on-lan magic ethernet packet */
#include <u.h>
#include <libc.h>
#include <ip.h>

enum {
	Eaddrlen = 6,	/* 48 bits */
};

typedef struct Wolpack Wolpack;
struct Wolpack{
	uchar	magic[6];
	uchar	macs[16][Eaddrlen];
	char	pass[6+1];
};

int verbose;

void
usage(void)
{
	fprint(2, "usage: wol [-v] [-a dialstr] [-c password] macaddr\n");
	exits("usage");
}

void
fillmac(Wolpack *w, uchar *mac)
{
	int i;

	for(i = 0; i < nelem(w->macs); i++)
		memmove(w->macs[i], mac, Eaddrlen);
}

void
dumppack(Wolpack *w)
{
	int i;

	print("packet: [\n");
	print("\t%E\n", w->magic);
	for(i = 0; i < nelem(w->macs); i++)
		print("\t%E\n", w->macs[i]);
	print("\t%6s\n", w->pass);
	print("]\n");
}

void
main(int argc, char* argv[])
{
	int fd, nw;
	char *argmac, *pass, *address;
	uchar mac[Eaddrlen];
	static Wolpack w = {
		.magic { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, }
	};

	address = pass = nil;
	fmtinstall('E', eipfmt);

	ARGBEGIN{
	case 'a':
		address = EARGF(usage());
		break;
	case 'c':
		pass = EARGF(usage());
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	argmac = argv[0];
	if(verbose)
		print("mac is %s, pass is %s\n", argmac, pass);

	parseether(mac, argmac);
	fillmac(&w, mac);
	if(pass){
		if(strlen(pass) > 6)
			sysfatal("password greater than 6 bytes");
		strcpy(w.pass, pass);
	}
	if(verbose)
		dumppack(&w);

	if(!address)
		address = "udp!255.255.255.255!0";

	fd = dial(address, nil, nil, nil);
	if(fd < 0)
		sysfatal("%s: %r", address);
	nw = write(fd, &w, sizeof w);
	if(nw != sizeof w)
		sysfatal("error sending: %r");
	exits(0);
}
