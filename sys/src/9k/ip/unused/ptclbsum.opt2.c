#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"ip.h"

static	short	endian	= 1;
static	uchar*	aendian	= (uchar*)&endian;
#define	LITTLE	*aendian

ushort
ptclbsum(register uchar *addr, int len)
{
	register ulong mdsum;
	register ushort *shp;
	ulong losum, hisum, odd;

	losum = hisum = mdsum = 0;
	odd = (uintptr)addr & 1;
	if(odd && len) {
		len--;
		hisum += *addr++;
	}
	shp = (ushort *)addr;
	while(len >= 8*2) {
		len -= 8*2;
		mdsum += shp[0] + shp[1] + shp[2] + shp[3] +
			 shp[4] + shp[5] + shp[6] + shp[7];
		shp += 8;
	}
	while(len >= 2) {
		len -= 2;
		mdsum += *shp++;
	}
	addr = (uchar *)shp;
	if(odd) {
		if(len)
			losum += addr[0];
		if(LITTLE)
			losum += mdsum;
		else
			hisum += mdsum;
	} else {
		if(len)
			hisum += addr[0];
		if(LITTLE)
			hisum += mdsum;
		else
			losum += mdsum;
	}

	losum += hisum >> 8;
	losum += (hisum & 0xff) << 8;
	while(hisum = losum>>16)
		losum = hisum + (losum & 0xffff);

	return losum;
}
