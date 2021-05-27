#include <u.h>
#include <libc.h>

u16int
le16get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+2;
	return (u16int)t[0] | (u16int)t[1]<<8;
}

u32int
le24get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+3;
	return (u32int)t[0] | (u32int)t[1]<<8 | (u32int)t[2]<<16;
}

u32int
le32get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+4;
	return (u32int)t[0] | (u32int)t[1]<<8 | (u32int)t[2]<<16 | (u32int)t[3]<<24;
}

u64int
le64get(uchar *t,  uchar **r)
{
	if(r != nil)
		*r = t+8;
	return (u64int)t[0] | (u64int)t[1]<<8 | (u64int)t[2]<<16 | (u64int)t[3]<<24 |
		(u64int)t[4]<<32 | (u64int)t[5]<<40 | (u64int)t[6]<<48 | (u64int)t[7]<<56;
}

uchar*
le16put(uchar *t, u16int r)
{
	*t++ = r;
	*t++ = r>>8;
	return t;
}

uchar*
le24put(uchar *t, u32int r)
{
	*t++ = r;
	*t++ = r>>8;
	*t++ = r>>16;
	return t;
}

uchar*
le32put(uchar *t, u32int r)
{
	*t++ = r;
	*t++ = r>>8;
	*t++ = r>>16;
	*t++ = r>>24;
	return t;
}

uchar*
le64put(uchar *t, u64int r)
{
	*t++ = r;
	*t++ = r>>8;
	*t++ = r>>16;
	*t++ = r>>24;
	*t++ = r>>32;
	*t++ = r>>40;
	*t++ = r>>48;
	*t++ = r>>56;
	return t;
}
