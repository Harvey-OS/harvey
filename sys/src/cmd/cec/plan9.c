/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright © Coraid, Inc. 2006.  All rights reserved. */
#include <u.h>
#include <libc.h>
#include "cec.h"

int fd = -1;
int cfd = -1;
int efd = -1;

int
netopen0(char* e)
{
	char buf[128], ctl[13];
	int n;

	snprint(buf, sizeof buf, "%s/clone", e);
	if((efd = open(buf, ORDWR)) == -1)
		return -1;
	memset(ctl, 0, sizeof ctl);
	if(read(efd, ctl, sizeof ctl) < 0)
		return -1;
	n = atoi(ctl);
	snprint(buf, sizeof buf, "connect %d", Etype);
	if(write(efd, buf, strlen(buf)) != strlen(buf))
		return -1;
	snprint(buf, sizeof buf, "%s/%d/ctl", e, n);
	if((cfd = open(buf, ORDWR)) < 0)
		return -1;
	snprint(buf, sizeof buf, "nonblocking");
	if(write(cfd, buf, strlen(buf)) != strlen(buf))
		return -1;
	snprint(buf, sizeof buf, "%s/%d/data", e, n);
	fd = open(buf, ORDWR);
	return fd;
}

void
netclose(void)
{
	close(efd);
	close(cfd);
	close(fd);
	efd = -1;
	cfd = -1;
	fd = -1;
}

int
netopen(char* e)
{
	int r;

	if((r = netopen0(e)) >= 0)
		return r;
	perror("netopen");
	netclose();
	return -1;
}

/* what if len < netlen? */
int
netget(void* v, int len)
{
	int l;

	l = read(fd, v, len);
	if(debug && l > 0) {
		fprint(2, "read %d bytes\n", l);
		dump((uint8_t*)v, l);
	}
	if(l <= 0)
		return 0;
	return l;
}

int
netsend(void* v, int len)
{
	uint8_t* p;

	p = v;
	if(debug) {
		fprint(2, "sending %d bytes\n", len);
		dump(p, len);
	}
	if(len < 60)
		len = 60; /* mintu */
	return write(fd, p, len);
}
