#include <u.h>
#include <libc.h>
#include "gzip.h"

static ulong	crctab[256];

void	mkcrctab(ulong);
ulong	blockcrc(ulong, void*, int);

void
mkcrctab(ulong poly)
{
	ulong crc;
	int i, j;

	for(i = 0; i < 256; i++){
		crc = i;
		for(j = 0; j < 8; j++){
			if(crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		crctab[i] = crc;
	}
}

ulong
blockcrc(ulong crc, void *vbuf, int n)
{
	int i;
	uchar *buf;

	crc ^= 0xffffffff;
	buf = vbuf;
	for(i = 0; i < n; i++)
		crc = crctab[(crc ^ *buf++) & 0xff] ^ (crc >> 8);
	return crc ^ 0xffffffff;
}
