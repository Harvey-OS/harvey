#include "all.h"
#include "io.h"
#include "mem.h"

#define bcd2dec(bcd)	(((((bcd)>>4) & 0x0F) * 10) + ((bcd) & 0x0F))
#define dec2bcd(dec)	((((dec)/10)<<4)|((dec)%10))

void
setrtc(long secs)
{
	RTCdev *dev = RTC;
	Rtc rtc;

	sec2rtc(secs, &rtc);
	dev->control |= RTCWRITE;
	dev->year = dec2bcd(rtc.year % 100);
	dev->mon = dec2bcd(rtc.mon);
	dev->mday = dec2bcd(rtc.mday);
	dev->hour = dec2bcd(rtc.hour);
	dev->min = dec2bcd(rtc.min);
	dev->sec = dec2bcd(rtc.sec);
	wbflush();
	dev->control &= ~RTCWRITE;
	wbflush();
}

long
rtctime(void)
{
	RTCdev *dev = RTC;
	Rtc rtc;

	dev->control |= RTCREAD;
	wbflush();
	rtc.sec = bcd2dec(dev->sec) & 0x7F;
	rtc.min = bcd2dec(dev->min & 0x7F);
	rtc.hour = bcd2dec(dev->hour & 0x3F);
	rtc.mday = bcd2dec(dev->mday & 0x3F);
	rtc.mon = bcd2dec(dev->mon & 0x3F);
	if (rtc.mon < 1 || rtc.mon > 12)
		return 0;
	rtc.year = bcd2dec(dev->year);
	dev->control &= ~RTCREAD;
	wbflush();

	/*
	 *  the world starts Jan 1 1970
	 */
	if(rtc.year < 70)
		rtc.year += 2000;
	else
		rtc.year += 1900;

	return rtc2sec(&rtc);
}

int
nvwrite(int offset, void *buf, int n)
{
	Nvram *nv;
	int i;

	if(offset > NVLEN)
		return 0;
	if(n > NVLEN - offset)
		n = NVLEN - offset;

	nv = NVRAM;
	for(i = 0; i < n; i++)
		nv[i+offset].val = ((char*)buf)[i];
	return n;
}

int
nvread(int offset, void *a, int n)
{
	Nvram *nv;
	char *p;
	int i;

	if(offset > NVLEN)
		return 0;
	if(n > NVLEN - offset)
		n = NVLEN - offset;

	nv = (Nvram *)NVRAM;
	p = a;
	for(i = 0; i < n; i++)
		p[i] = nv[i+offset].val;

	return n;
}
