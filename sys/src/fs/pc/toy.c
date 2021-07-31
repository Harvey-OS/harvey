#include "all.h"
#include "io.h"
#include "mem.h"

enum {
	Paddr=		0x70,	/* address port */
	Pdata=		0x71,	/* data port */

	Seconds=	0x00,
	Minutes=	0x02,
	Hours=		0x04, 
	Mday=		0x07,
	Month=		0x08,
	Year=		0x09,
	Status=		0x0A,

	Nbcd=		6,
};

#define GETBCD(o)	((bcdclock[o]&0xf) + 10*(bcdclock[o]>>4))
#define PUTBCD(n,o)	bcdclock[o] = (n % 10) | (((n / 10) % 10)<<4)

static Lock rtclock;

void
setrtc(ulong secs)
{
	Rtc rtc;
	uchar bcdclock[Nbcd];

	sec2rtc(secs, &rtc);

	PUTBCD(rtc.sec, 0);
	PUTBCD(rtc.min, 1);
	PUTBCD(rtc.hour, 2);
	PUTBCD(rtc.mday, 3);
	PUTBCD(rtc.mon, 4);
	PUTBCD(rtc.year, 5);

	ilock(&rtclock);
	outb(Paddr, Seconds);	outb(Pdata, bcdclock[0]);
	outb(Paddr, Minutes);	outb(Pdata, bcdclock[1]);
	outb(Paddr, Hours);	outb(Pdata, bcdclock[2]);
	outb(Paddr, Mday);	outb(Pdata, bcdclock[3]);
	outb(Paddr, Month);	outb(Pdata, bcdclock[4]);
	outb(Paddr, Year);	outb(Pdata, bcdclock[5]);
	iunlock(&rtclock);
}

static ulong	 
_rtctime(void)
{
	uchar bcdclock[Nbcd];
	Rtc rtc;
	int i;

	/* don't do the read until the clock is no longer busy */
	for(i = 0; i < 10000; i++){
		outb(Paddr, Status);
		if(inb(Pdata) & 0x80)
			continue;

		/* read clock values */
		outb(Paddr, Seconds);	bcdclock[0] = inb(Pdata);
		outb(Paddr, Minutes);	bcdclock[1] = inb(Pdata);
		outb(Paddr, Hours);	bcdclock[2] = inb(Pdata);
		outb(Paddr, Mday);	bcdclock[3] = inb(Pdata);
		outb(Paddr, Month);	bcdclock[4] = inb(Pdata);
		outb(Paddr, Year);	bcdclock[5] = inb(Pdata);

		outb(Paddr, Status);
		if((inb(Pdata) & 0x80) == 0)
			break;
	}

	/*
	 *  convert from BCD
	 */
	rtc.sec = GETBCD(0);
	rtc.min = GETBCD(1);
	rtc.hour = GETBCD(2);
	rtc.mday = GETBCD(3);
	rtc.mon = GETBCD(4);
	rtc.year = GETBCD(5);

	/*
	 *  the world starts jan 1 1970
	 */
	if(rtc.year < 70)
		rtc.year += 2000;
	else
		rtc.year += 1900;
	return rtc2sec(&rtc);
}

ulong
rtctime(void)
{
	int i;
	ulong t, ot;

	ilock(&rtclock);

	/* loop till we get two reads in a row the same */
	t = _rtctime();
	for(i = 0; i < 100; i++){
		ot = t;
		t = _rtctime();
		if(ot == t)
			break;
	}
	iunlock(&rtclock);

	return t;
}

uchar
nvramread(int offset)
{
	outb(Paddr, offset);
	return inb(Pdata);
}
