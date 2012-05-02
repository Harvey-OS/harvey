#include <lib9.h>
#include <bio.h>
#include "mach.h"

/*
 * big-endian short
 */
ushort
beswab(ushort s)
{
	uchar *p;

	p = (uchar*)&s;
	return (p[0]<<8) | p[1];
}

/*
 * big-endian long
 */
ulong
beswal(ulong l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

/*
 * big-endian vlong
 */
uvlong
beswav(uvlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((uvlong)p[0]<<56) | ((uvlong)p[1]<<48) | ((uvlong)p[2]<<40)
				  | ((uvlong)p[3]<<32) | ((uvlong)p[4]<<24)
				  | ((uvlong)p[5]<<16) | ((uvlong)p[6]<<8)
				  | (uvlong)p[7];
}

/*
 * little-endian short
 */
ushort
leswab(ushort s)
{
	uchar *p;

	p = (uchar*)&s;
	return (p[1]<<8) | p[0];
}

/*
 * little-endian long
 */
ulong
leswal(ulong l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
}

/*
 * little-endian vlong
 */
uvlong
leswav(uvlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((uvlong)p[7]<<56) | ((uvlong)p[6]<<48) | ((uvlong)p[5]<<40)
				  | ((uvlong)p[4]<<32) | ((uvlong)p[3]<<24)
				  | ((uvlong)p[2]<<16) | ((uvlong)p[1]<<8)
				  | (uvlong)p[0];
}
