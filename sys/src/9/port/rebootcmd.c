/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include 	<elf.h>

/* some day we will want these.
void
readn(Chan *c, void *vp, int32_t n)
{
	char *p;
	int32_t nn;

	p = vp;
	while(n > 0) {
		nn = c->dev->read(c, p, n, c->offset);
		if(nn == 0)
			error(Eshort);
		c->offset += nn;
		p += nn;
		n -= nn;
	}
}

void
setbootcmd(int argc, char *argv[])
{
	char *buf, *p, *ep;
	int i;

	buf = malloc(1024);
	if(buf == nil)
		error(Enomem);
	p = buf;
	ep = buf + 1024;
	for(i=0; i<argc; i++)
		p = seprint(p, ep, "%q ", argv[i]);
	*p = 0;
	ksetenv("bootcmd", buf, 1);
	free(buf);
}

*/
void
rebootcmd(int argc, char *argv[])
{
	error("Reboot with a file is not supported yet");
}
