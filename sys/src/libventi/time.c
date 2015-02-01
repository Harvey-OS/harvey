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
#include <venti.h>

int
vttimefmt(Fmt *fmt)
{
	vlong ns;
	Tm tm;

	if(fmt->flags&FmtLong){
		ns = nsec();
		tm = *localtime(ns/1000000000);
		return fmtprint(fmt, "%04d/%02d%02d %02d:%02d:%02d.%03d", 
			tm.year+1900, tm.mon+1, tm.mday, 
			tm.hour, tm.min, tm.sec,
			(int)(ns%1000000000)/1000000);
	}else{
		tm = *localtime(time(0));
		return fmtprint(fmt, "%04d/%02d%02d %02d:%02d:%02d", 
			tm.year+1900, tm.mon+1, tm.mday, 
			tm.hour, tm.min, tm.sec);
	}
}

