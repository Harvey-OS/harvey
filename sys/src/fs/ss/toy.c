#include "all.h"
#include "io.h"
#include "mem.h"

static struct {
	uchar	*ram;
	RTCdev	*rtc;
} ioaddr;

#define bcd2dec(bcd)	(((((bcd)>>4) & 0x0F) * 10) + ((bcd) & 0x0F))
#define dec2bcd(dec)	((((dec)/10)<<4)|((dec)%10))

void
toyinit(void)
{
	KMap *k;

	k = kmappa(NVRAM, PTENOCACHE|PTEIO);
	ioaddr.ram = (uchar*)k->va;
	ioaddr.rtc = (RTCdev*)(k->va+RTCOFF);
}

void
setrtc(long secs)
{
	RTCdev *dev = ioaddr.rtc;
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
	RTCdev *dev = ioaddr.rtc;
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
	if(offset > NVWRITE)
		return 0;
	if(n > NVWRITE - offset)
		n = NVWRITE - offset;

	memmove(ioaddr.ram+offset, buf, n);

	return n;
}

int
nvread(int offset, void *buf, int n)
{
	if(offset > NVREAD)
		return 0;
	if(n > NVREAD - offset)
		n = NVREAD - offset;

	memmove(buf, ioaddr.ram+offset, n);

	return n;
}
