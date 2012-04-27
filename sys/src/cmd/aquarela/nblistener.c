#include "headers.h"

static char *hmsg = "headers";

int nbudphdrsize;

char *
nbudpannounce(ushort port, int *fdp)
{
	int data, ctl;
	char dir[64], datafile[64+6], addr[NETPATHLEN];

	snprint(addr, sizeof(addr), "udp!*!%d", port);
	/* get a udp port */
	ctl = announce(addr, dir);
	if(ctl < 0)
		return "can't announce on port";
	snprint(datafile, sizeof(datafile), "%s/data", dir);

	/* turn on header style interface */
	nbudphdrsize = Udphdrsize;
	if (write(ctl, hmsg, strlen(hmsg)) != strlen(hmsg))
		return "failed to turn on headers";
	data = open(datafile, ORDWR);
	if (data < 0) {
		close(ctl);
		return "failed to open data file";
	}
	close(ctl);
	*fdp = data;
	return nil;
}
