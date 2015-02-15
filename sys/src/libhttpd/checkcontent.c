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
#include <bin.h>
#include <httpd.h>

int
hcheckcontent(HContent *me, HContent *oks, char *list, int size)
{
	HContent *ok;

	if(oks == nil || me == nil)
		return 1;
	for(ok = oks; ok != nil; ok = ok->next){
		if((cistrcmp(ok->generic, me->generic) == 0 || strcmp(ok->generic, "*") == 0)
		&& (me->specific == nil || cistrcmp(ok->specific, me->specific) == 0 || strcmp(ok->specific, "*") == 0)){
			if(ok->mxb > 0 && size > ok->mxb)
				return 0;
			return 1;
		}
	}

	USED(list);
	if(0){
		fprint(2, "list: %s/%s not found\n", me->generic, me->specific);
		for(; oks != nil; oks = oks->next){
			if(oks->specific)
				fprint(2, "\t%s/%s\n", oks->generic, oks->specific);
			else
				fprint(2, "\t%s\n", oks->generic);
		}
	}
	return 0;
}
