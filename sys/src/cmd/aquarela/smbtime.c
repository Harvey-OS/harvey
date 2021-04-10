/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

void
smbplan9time2datetime(u32 time, int tzoff, u16 *datep,
		      u16 *timep)
{
	Tm *tm;
	if (tzoff < 0)
		time -= (u32)-tzoff;
	else
		time += tzoff;
	tm = gmtime(time);
	*datep = (tm->mday) | ((tm->mon + 1) << 5) | ((tm->year - 80) << 9);
	*timep = (tm->sec >> 1) | (tm->min << 5) | (tm->hour << 11);
}

u32
smbdatetime2plan9time(u16 date, u16 time, int tzoff)
{
	Tm tm;
	strcpy(tm.zone, "GMT");
	tm.mday = date & 0x1f;
	tm.mon = ((date >> 5) & 0xf) - 1;
	tm.year = (date >> 9) + 80;
	tm.yday = 0;
	tm.sec = (time & 0x1f) << 1;
	tm.min = (time >> 5) & 0x3f;
	tm.hour = time >> 11;
	smblogprint(-1, "smbdatetime2plan9time: converting %d/%d/%d %d:%d:%d\n",
		tm.year + 1900, tm.mon + 1, tm.mday, tm.hour, tm.min, tm.sec);
	return tm2sec(&tm) - tzoff;
}

i64
smbplan9time2time(u32 time)
{
	return ((i64)time + 11644473600LL) * 10000000;
}

u32
smbtime2plan9time(i64 nttime)
{
	return (nttime / 10000000 - 11644473600LL);
}

u32
smbplan9time2utime(u32 time, int tzoff)
{
	if (tzoff < 0)
		time -= (u32)-tzoff;
	else
		time += tzoff;
	return time;
}

u32
smbutime2plan9time(u32 utime, int tzoff)
{
	if (tzoff < 0)
		utime += (u32)-tzoff;
	else
		utime -= tzoff;
	return utime;
}
