#include "headers.h"

void
smbplan9time2datetime(ulong time, int tzoff, ushort *datep, ushort *timep)
{
	Tm *tm;
	if (tzoff < 0)
		time -= (ulong)-tzoff;
	else
		time += tzoff;
	tm = gmtime(time);
	*datep = (tm->mday) | ((tm->mon + 1) << 5) | ((tm->year - 80) << 9);
	*timep = (tm->sec >> 1) | (tm->min << 5) | (tm->hour << 11);
}

ulong
smbdatetime2plan9time(ushort date, ushort time, int tzoff)
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

vlong
smbplan9time2time(ulong time)
{
	return ((vlong)time + 11644473600LL) * 10000000;
}

ulong
smbtime2plan9time(vlong nttime)
{
	return (nttime / 10000000 - 11644473600LL);
}

ulong
smbplan9time2utime(ulong time, int tzoff)
{
	if (tzoff < 0)
		time -= (ulong)-tzoff;
	else
		time += tzoff;
	return time;
}

ulong
smbutime2plan9time(ulong utime, int tzoff)
{
	if (tzoff < 0)
		utime += (ulong)-tzoff;
	else
		utime -= tzoff;
	return utime;
}
