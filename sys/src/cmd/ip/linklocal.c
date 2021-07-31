/*
 * linklocal - print ipv6 linklocal address of a mac address
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

void
ea2lla(uchar *lla, uchar *ea)
{
	assert(IPaddrlen == 16);
	memset(lla, 0, IPaddrlen);
	lla[0]  = 0xFE;
	lla[1]  = 0x80;
	lla[8]  = ea[0] | 0x2;
	lla[9]  = ea[1];
	lla[10] = ea[2];
	lla[11] = 0xFF;
	lla[12] = 0xFE;
	lla[13] = ea[3];
	lla[14] = ea[4];
	lla[15] = ea[5];
}

static void
process(char *ether)
{
	uchar ethaddr[6], ipaddr[16];

	if (parseether(ethaddr, ether) < 0)
		sysfatal("%s: not an ether address\n", ether);
	ea2lla(ipaddr, ethaddr);
	print("%I\n", ipaddr);
}

void
main(int argc, char *argv[])
{
	int i, errflg = 0;

	ARGBEGIN {
	default:
		errflg++;
		break;
	} ARGEND

	fmtinstall('I', eipfmt);
	if (argc <= 0 || errflg)
		sysfatal("usage: %s ether...", argv0);

	for (i = 0; i < argc; i++)
		process(argv[i]);
	exits(0);
}
