#include <u.h>
#include <libc.h>

u16int
be16get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+2;
	return (u16int)t[0]<<8 | (u16int)t[1];
}

u32int
be24get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+3;
	return (u32int)t[1]<<16 | (u32int)t[2]<<8 | (u32int)t[3];
}

u32int
be32get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+4;
	return (u32int)t[0]<<24 | (u32int)t[1]<<16 | (u32int)t[2]<<8 | (u32int)t[3];
}

u64int
be64get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+8;
	return (u64int)t[0]<<56 | (u64int)t[1]<<48 | (u64int)t[2]<<40 | (u64int)t[3]<<32 |
		(u64int)t[4]<<24 | (u64int)t[5]<<16 | (u64int)t[6]<<8 | (u64int)t[7];
}

uchar*
be16put(uchar *t, u16int r)
{
	*t++ = r>>8;
	*t++ = r;
	return t;
}

uchar*
be32put(uchar *t, u32int r)
{
	*t++ = r>>24;
	*t++ = r>>16;
	*t++ = r>>8;
	*t++ = r;
	return t;
}

uchar*
be24put(uchar *t, u32int r)
{
	*t++ = r>>16;
	*t++ = r>>8;
	*t++ = r;
	return t;
}

uchar*
be64put(uchar *t, u64int r)
{
	*t++ = r>>56;
	*t++ = r>>48;
	*t++ = r>>40;
	*t++ = r>>32;
	*t++ = r>>24;
	*t++ = r>>16;
	*t++ = r>>8;
	*t++ = r;
	return t;
}

