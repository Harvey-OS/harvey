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
	register ulong mdsum, t1, t2;
	register ushort *shp;
	ulong losum, hisum, x;

	losum = hisum = mdsum = 0;
	x = (uintptr)addr & 1;
	if(x) {
		if(len) {
			len--;
			hisum += *addr++;
		}
	}
	shp = (ushort *)addr;
	while(len >= 8*2) {
		len -= 8*2;
		t1 = *shp++;
		t2 = *shp++;	mdsum += t1;
		t1 = *shp++;	mdsum += t2;
		t2 = *shp++;	mdsum += t1;
		t1 = *shp++;	mdsum += t2;
		t2 = *shp++;	mdsum += t1;
		t1 = *shp++;	mdsum += t2;
		t2 = *shp++;	mdsum += t1;
		mdsum += t2;
	}
	while(len >= 2) {
		len -= 2;
		mdsum += *shp++;
	}
	addr = (uchar *)shp;
	if(x) {
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

	return losum & 0xffff;
}
