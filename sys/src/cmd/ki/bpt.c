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
#include <ctype.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sparc.h"

void
dobplist(void)
{
	Breakpoint *b;
	char buf[512];

	for(b = bplist; b; b = b->next) {
		switch(b->type) {
		case Instruction:
			Bprint(bioout, "0x%lux,%d:b %d done, at ", b->addr, b->count, b->done);
			symoff(buf, sizeof(buf), b->addr, CTEXT);
			Bprint(bioout, "%s", buf);
			break;

		case Access:
			Bprint(bioout, "0x%lux,%d:ba %d done, at ", b->addr, b->count, b->done);
			symoff(buf, sizeof(buf), b->addr, CDATA);
			Bprint(bioout, "%s", buf);
			break;

		case Read:
			Bprint(bioout, "0x%lux,%d:br %d done, at ", b->addr, b->count, b->done);
			symoff(buf, sizeof(buf), b->addr, CDATA);
			Bprint(bioout, "%s", buf);
			break;

		case Write:
			Bprint(bioout, "0x%lux,%d:bw %d done, at ", b->addr, b->count, b->done);
			symoff(buf, sizeof(buf), b->addr, CDATA);
			Bprint(bioout, "%s", buf);
			break;

		case Equal:
			Bprint(bioout, "0x%lux,%d:be at ", b->addr, b->count);
			symoff(buf, sizeof(buf), b->addr, CDATA);
			Bprint(bioout, "%s", buf);
			break;
		}
		Bprint(bioout, "\n");
	}
}

void
breakpoint(char *addr, char *cp)
{
	Breakpoint *b;
	int type;

	cp = nextc(cp);
	type = Instruction;
	switch(*cp) {
	case 'r':
		membpt++;
		type = Read;
		break;
	case 'a':
		membpt++;
		type = Access;
		break;
	case 'w':
		membpt++;
		type = Write;
		break;
	case 'e':
		membpt++;
		type = Equal;
		break;
	}
	b = emalloc(sizeof(Breakpoint));
	b->addr = expr(addr);
	b->type = type;
	b->count = cmdcount;
	b->done = cmdcount;

	b->next = bplist;
	bplist = b;
}

void
delbpt(char *addr)
{
	Breakpoint *b, **l;
	ulong baddr;

	baddr = expr(addr);
	l = &bplist;
	for(b = *l; b; b = b->next) {
		if(b->addr == baddr) {
			if(b->type != Instruction)
				membpt++;
			*l = b->next;
			free(b);
			return;
		}
		l = &b->next;	
	}

	Bprint(bioout, "no breakpoint\n");
}

void
brkchk(ulong addr, int type)
{
	Breakpoint *b;

	for(b = bplist; b; b = b->next) {
		if(b->addr == addr && (b->type&type)) {
			if(b->type == Equal && getmem_4(addr) == b->count) {
				count = 1;
				atbpt = 1;
				return;
			}
			if(--b->done == 0) {
				b->done = b->count;
				count = 1;
				atbpt = 1;
				return;
			}
		}
	}	
}
