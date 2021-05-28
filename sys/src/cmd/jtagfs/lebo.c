#include <u.h>
#include <libc.h>
#include <lebo.h>

void
hleputv(void *p, uvlong v)
{
	uchar *a;

	a = p;
	a[7] = v>>56;
	a[6] = v>>48;
	a[5] = v>>40;
	a[4] = v>>32;
	a[3] = v>>24;
	a[2] = v>>16;
	a[1] = v>>8;
	a[0] = v;
}

void
hleputl(void *p, uint v)
{
	uchar *a;

	a = p;
	a[3] = v>>24;
	a[2] = v>>16;
	a[1] = v>>8;
	a[0] = v;
}

void
hleputs(void *p, ushort v)
{
	uchar *a;

	a = p;
	a[1] = v>>8;
	a[0] = v;
}

uvlong
lehgetv(void *p)
{
	uchar *a;
	uvlong v;

	a = p;
	v = (uvlong)a[7]<<56;
	v |= (uvlong)a[6]<<48;
	v |= (uvlong)a[5]<<40;
	v |= (uvlong)a[4]<<32;
	v |= a[3]<<24;
	v |= a[2]<<16;
	v |= a[1]<<8;
	v |= a[0]<<0;
	return v;
}

uint
lehgetl(void *p)
{
	uchar *a;

	a = p;
	return (a[3]<<24)|(a[2]<<16)|(a[1]<<8)|(a[0]<<0);
}

ushort
lehgets(void *p)
{
	uchar *a;

	a = p;
	return (a[1]<<8)|(a[0]<<0);
}
