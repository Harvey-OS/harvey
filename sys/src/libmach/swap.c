#include <u.h>

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
long
beswal(long l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

/*
 * big-endian vlong
 */
vlong
beswav(vlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((vlong)p[0]<<56) | ((vlong)p[1]<<48) | ((vlong)p[2]<<40)
				 | ((vlong)p[3]<<32) | ((vlong)p[4]<<24)
				 | ((vlong)p[5]<<16) | ((vlong)p[6]<<8)
				 | (vlong)p[7];
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
long
leswal(long l)
{
	uchar *p;

	p = (uchar*)&l;
	return (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
}

/*
 * little-endian vlong
 */
vlong
leswav(vlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((vlong)p[7]<<56) | ((vlong)p[6]<<48) | ((vlong)p[5]<<40)
				 | ((vlong)p[4]<<32) | ((vlong)p[3]<<24)
				 | ((vlong)p[2]<<16) | ((vlong)p[1]<<8)
				 | (vlong)p[0];
}
