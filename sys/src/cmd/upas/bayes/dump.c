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
#include <regexp.h>
#include "/sys/src/libregexp/regcomp.h"
#include "dfa.h"

#define DUMP

void
dump(Dreprog *pp)
{
	int i, j;
	Dreinst *l;

	print("start %ld %ld %ld %ld\n",
		pp->start[0]-pp->inst,
		pp->start[1]-pp->inst,
		pp->start[2]-pp->inst,
		pp->start[3]-pp->inst);

	for(i=0; i<pp->ninst; i++){
		l = &pp->inst[i];
		print("%d:", i);
		for(j=0; j<l->nc; j++){
			print(" [");
			if(j == 0)
				if(l->c[j].start > 1)
					print("<bad start %d>\n", l->c[j].start);
			if(j != 0)
				print("%C%s", l->c[j].start&0xFFFF, (l->c[j].start&0x10000) ? "$" : "");
			print("-");
			if(j != l->nc-1)
				print("%C%s", (l->c[j+1].start&0xFFFF)-1, (l->c[j+1].start&0x10000) ? "$" : "");
			print("] %ld", l->c[j].next - pp->inst);
		}
		if(l->isfinal)
			print(" final");
		if(l->isloop)
			print(" loop");
		print("\n");
	}
}


void
main(int argc, char **argv)
{
	int i;
	Reprog *p;
	Dreprog *dp;

	i = 1;
		p = regcomp(argv[i]);
		if(p == 0){
			print("=== %s: bad regexp\n", argv[i]);
		}
	//	print("=== %s\n", argv[i]);
	//	rdump(p);
		dp = dregcvt(p);
		print("=== dfa\n");
		dump(dp);
	
	for(i=2; i<argc; i++)
		print("match %d\n", dregexec(dp, argv[i], 1));
	exits(0);
}
