/*
 * Inmos G17x d/a converter
 */

typedef
struct	G170
{
	uchar	waddr;
	uchar	value;
	uchar	mask;
	uchar	raddr;
} G170;

#define DAC	((G170*)0xc0100000)
