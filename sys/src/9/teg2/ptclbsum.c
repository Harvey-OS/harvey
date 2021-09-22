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
ptclbsum(uchar *addr, int len)
{
	ulong mdsum, losum, hisum, x;

	losum = hisum = mdsum = 0;
	x = (uintptr)addr & 1;
	if(x && len) {
		len--;
		hisum = *addr++;
	}
	for(; len >= 16; len -= 16, addr += 16) {
		ushort *saddr = (ushort *)addr;
	
		mdsum += saddr[0] + saddr[1] + saddr[2] + saddr[3] +
			saddr[4] + saddr[5] + saddr[6] + saddr[7];
	}
	for(; len >= 2; len -= 2, addr += 2)
		mdsum += *(ushort*)addr;
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

	losum += (hisum >> 8) + ((hisum & 0xff) << 8);
	while(hisum = losum>>16)
		losum = hisum + (losum & 0xffff);

	return losum & 0xffff;
}
