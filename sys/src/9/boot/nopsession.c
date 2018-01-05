/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "../boot/boot.h"

static Fcall	hdr;

static void
rpc(int fd, int type)
{
	int n, l;
	char buf[128], *p;

	hdr.type = type;
	hdr.tag = NOTAG;
	n = convS2M(&hdr, buf);
	if(write(fd, buf, n) != n)
		fatal("write rpc");

	print("...");
	p = buf;
	l = 0;
	while(l < 3) {
		n = read(fd, p, 3);
		if(n <= 0)
			fatal("read rpc");
		if(n == 2 && l == 0 && buf[0] == 'O' && buf[1] == 'K')
			continue;
		p += n;
		l += n;
	}
	if(convM2S(buf, &hdr, n) == 0){
		print("%x %x %x\n", buf[0], buf[1], buf[2]);
		fatal("rpc format");
	}
	if(hdr.tag != NOTAG)
		fatal("rpc tag not NOTAG");
	if(hdr.type == Rerror){
		print("error %s;", hdr.ename);
		fatal("remote error");
	}
	if(hdr.type != type+1)
		fatal("not reply");
}

void
nop(int fd)
{
	print("nop");
	rpc(fd, Tnop);
}
