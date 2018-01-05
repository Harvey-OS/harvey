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
#include <bio.h>
#include "snap.h"

char *pfile[Npfile] = {
	[Psegment]	"segment",
	[Pfd]			"fd",
	[Pfpregs]		"fpregs",
	[Pnoteid]		"noteid",
	[Pkregs]		"kregs",
	[Pns]			"ns",
	[Pproc]		"proc",
	[Pregs]		"regs",
	[Pstatus]		"status",
};

static void
writeseg(Biobuf *b, Proc *proc, Seg *s)
{
	int i, npg;
	Page **pp, *p;
	int type;

	if(s == nil){
		Bprint(b, "%-11u %-11u ", 0, 0);
		return;
	}

	type = proc->text ==  s ? 't' : 'm';
	npg = (s->len+Pagesize-1)/Pagesize;
	if(npg != s->npg)
		abort();

	Bprint(b, "%-11llu %-11llu ", s->offset, s->len);
	if(s->len == 0)
		return;

	for(i=0, pp=s->pg, p=*pp; i<npg; i++, pp++, p=*pp) {
		if(p->written) {
			if(p->type == 'z') {
				Bprint(b, "z");
				continue;
			}
			Bprint(b, "%c%-11ld %-11llu ", p->type, p->pid, p->offset);
		} else {
			Bprint(b, "r");
			Bwrite(b, p->data, p->len);
			if(p->len != Pagesize && i != npg-1)
				abort();
			p->written = 1;
			p->type = type;
			p->offset = s->offset + i*Pagesize;
			p->pid = proc->pid;
		}
	}
}


void
writesnap(Biobuf *b, Proc *p)
{
	int i, n;
	Data *d;

	for(i=0; i<Npfile; i++)
		if(d = p->d[i]) {
			Bprint(b, "%-11ld %s\n%-11lu ", p->pid, pfile[i], d->len);
			Bwrite(b, d->data, d->len);
		}

	if(p->text) {
		Bprint(b, "%-11ld text\n", p->pid);
		writeseg(b, p, p->text);
	}

	if(n = p->nseg) {
		Bprint(b, "%-11ld mem\n%-11d ", p->pid, n);
		for(i=0; i<n; i++)
			writeseg(b, p, p->seg[i]);
	}
}
