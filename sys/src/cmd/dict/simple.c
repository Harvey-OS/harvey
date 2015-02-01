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
#include "dict.h"

/*
 * Routines for handling dictionaries in UTF, headword
 * separated from entry by tab, entries separated by newline.
 */

void
simpleprintentry(Entry e, int cmd)
{
	uchar *p, *pe;

	p = (uchar *)e.start;
	pe = (uchar *)e.end;
	while(p < pe){
		if(*p == '\t'){
			if(cmd == 'h')
				break;
			else
				outchar(' '), ++p;
		}else if(*p == '\n')
			break;
		else
			outchar(*p++);
	}
	outnl(0);
}

long
simplenextoff(long fromoff)
{
	if(Bseek(bdict, fromoff, 0) < 0)
		return -1;
	if(Brdline(bdict, '\n') == 0)
		return -1;
	return Boffset(bdict);
}

void
simpleprintkey(void)
{
	Bprint(bout, "No pronunciation key.\n");
}
