#include "all.h"
#include "io.h"
#include "mem.h"

#define GETBCD(o) ((bcdclock[o]&0xf) + 10*(bcdclock[o]>>4))
#define PUTBCD(n,o) bcdclock[o] = (n % 10) | (((n / 10) % 10)<<4)

enum
{
	Nbcd = 8	/* number of bcd bytes in the clock */
};

uchar	pattern[] =
{
	0xc5, 0x3a, 0xa3, 0x5c, 0xc5, 0x3a, 0xa3, 0x5c
};

typedef struct Nvram	Nvram;

/*
 * length of nvram is 2048
 * the last addresses are reserved by convention for
 * a checksum and setting the real time clock
 */
enum
{
	NVLEN	= 2046,
	NVCKSUM = 2046,
	NVRTC	= 2047,
};

struct Nvram
{
	uchar	val;
	uchar	pad[7];
};

static void
rtcpattern(void)
{
	uchar *nv, ch;
	int i, j;

	nv = RTC;

	/*
	 *  read the pattern sequence pointer to reset it
	 */
	ch = *nv;
	USED(ch);

	/*
	 *  stuff the pattern recognition codes one bit at
	 *  a time into *nv.
	 */
	for(i = 0; i < Nbcd; i++) {
		ch = pattern[i];
		for (j = 0; j < 8; j++) {
			*nv = ch & 1;
			ch >>= 1;
		}
	}
}

void
setrtc(long secs)
{
	Rtc rtc;
	int i, j;
	uchar bcdclock[Nbcd], ch, *nv;

	/*
	 *  convert to bcd
	 */
	sec2rtc(secs, &rtc);
	bcdclock[0] = bcdclock[4] = 0;
	PUTBCD(rtc.sec, 1);
	PUTBCD(rtc.min, 2);
	PUTBCD(rtc.hour, 3);
	PUTBCD(rtc.mday, 5);
	PUTBCD(rtc.mon, 6);
	PUTBCD(rtc.year, 7);

	/*
	 *  set up the pattern for the clock
	 */
	rtcpattern();

	/*
	 *  write the clock one bit at a time
	 */
	nv = RTC;
	for (i = 0; i < Nbcd; i++) {
		ch = bcdclock[i];
		for (j = 0; j < 8; j++) {
			*nv = ch & 1;
			ch >>= 1;
		}
	}
}

/*
 * real time clock.
 * an attempt is made to keep
 * this accurate
 */
long
rtctime(void)
{
	Rtc rtc;
	int i,j;
	uchar bcdclock[Nbcd], ch, *nv;

	nv = RTC;

	/*
	 *  set up the pattern for the clock
	 */
	rtcpattern();

	/*
	 *  read out the clock one bit at a time
	 */
	for (i = 0; i < Nbcd; i++) {
		ch = 0;
		for (j = 0; j < 8; j++)
			ch |= ((*nv & 0x1) << j);
		bcdclock[i] = ch;
	}

	/*
	 *  see if the clock oscillator is on
	 */
	if(bcdclock[4] & 0x20)
		return 0;		/* nope, time is bogus */

	/*
	 *  convert from BCD
	 */
	rtc.sec = GETBCD(1);
	rtc.min = GETBCD(2);
	rtc.hour = GETBCD(3);
	rtc.mday = GETBCD(5);
	rtc.mon = GETBCD(6);
	rtc.year = GETBCD(7);

	/*
	 *  the world starts jan 1 1970
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
	uchar cksum;
	int i;

	if(offset > NVLEN)
		return 0;
	if(n > NVLEN - offset)
		n = NVLEN - offset;

	nv = (Nvram *)NVRAM;
	for(i = 0; i < n; i++)
		nv[i+offset].val = ((char*)buf)[i];

	/*
	 * Seed the checksum so all-zeroes (all-ones) nvram doesn't have a zero
	 * (all-ones) checksum.
	 */
	cksum = 0xa5;
	nv = (Nvram *)NVRAM;
	for(i = 0; i < NVLEN; i++){
		cksum ^= nv[i].val;
		cksum = (cksum << 1) | ((cksum >> 7) & 1);
	}
	nv[NVCKSUM].val = cksum;
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
