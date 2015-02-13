/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

static int8_t *hmsg = "headers";

int nbudphdrsize;

int8_t *
nbudpannounce(uint16_t port, int *fdp)
{
	int data, ctl;
	int8_t dir[64], datafile[64+6], addr[NETPATHLEN];

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
