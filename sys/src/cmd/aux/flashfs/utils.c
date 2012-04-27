#include <u.h>
#include <libc.h>
#include "flashfs.h"

char*	prog;
ulong	sectsize;
ulong	nsects;
uchar*	sectbuff;
int	readonly;
ulong	delta;
int	eparity;

uchar	magic[]	= { MAGIC0, MAGIC1, MAGIC2, FFSVERS };

int
putc3(uchar *buff, ulong v)
{
	if(v < (1 << 7)) {
		buff[0] = v;
		return 1;
	}

	if(v < (1 << 13)) {
		buff[0] = 0x80 | (v >> 7);
		buff[1] = v & ((1 << 7) - 1);
		return 2;
	}

	if(v < (1 << 21)) {
		buff[0] = 0x80 | (v >> 15);
		buff[1] = 0x80 | ((v >> 8) & ((1 << 7) - 1));
		buff[2] = v;
		return 3;
	}

	fprint(2, "%s: putc3 fail 0x%lux, called from %#p\n", prog, v, getcallerpc(&buff));
	abort();
	return -1;
}

int
getc3(uchar *buff, ulong *p)
{
	int c, d;

	c = buff[0];
	if((c & 0x80) == 0) {
		*p = c;
		return 1;
	}

	c &= ~0x80;
	d = buff[1];
	if((d & 0x80) == 0) {
		*p = (c << 7) | d;
		return 2;
	}

	d &= ~0x80;
	*p = (c << 15) | (d << 8) | buff[2];
	return 3;
}

ulong
get4(uchar *b)
{
	return	(b[0] <<  0) |
		(b[1] <<  8) |
		(b[2] << 16) |
		(b[3] << 24);
}

void
put4(uchar *b, ulong v)
{
	b[0] = v >>  0;
	b[1] = v >>  8;
	b[2] = v >> 16;
	b[3] = v >> 24;
}
