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

#include	<a.out.h>

static uint32_t
l2be(int32_t l)
{
	uint8_t *cp;

	cp = (uint8_t*)&l;
	return (cp[0]<<24) | (cp[1]<<16) | (cp[2]<<8) | cp[3];
}


static void
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

static void
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

void
rebootcmd(int argc, char *argv[])
{
	Mach *m = machp();
	Chan *c;
	Exec exec;
	uintptr_t magic, text, rtext, entry, data, size;
	uint8_t *p;

	if(argc == 0)
		exit(0);

	panic("Reboot with a file is not supported yet");
	c = namec(argv[0], Aopen, OEXEC, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}

	readn(c, &exec, sizeof(Exec));
	magic = l2be(exec.magic);
	entry = l2be(exec.entry);
	text = l2be(exec.text);
	data = l2be(exec.data);
	if(magic != AOUT_MAGIC /*|| magic != ELF_MAGIC*/)
		error(Ebadexec);

	/* round text out to page boundary */
	rtext = BIGPGROUND(entry+text)-entry;
	size = rtext + data;
	p = malloc(size);
	if(p == nil)
		error(Enomem);

	if(waserror()){
		free(p);
		nexterror();
	}

	memset(p, 0, size);
	readn(c, p, text);
	readn(c, p + rtext, data);

	ksetenv("bootfile", argv[0], 1);
	setbootcmd(argc-1, argv+1);

	reboot((void*)entry, p, size);

	panic("return from reboot!");
}
