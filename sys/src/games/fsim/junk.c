#include	"type.h"

long
fmul(Fract f, long l)
{
	long x;

	x = ((double)f / (double)ONE) * (double)l;
	return x;
}

Fract
fdiv(long n, long d)
{
	long x;

	x = ((double)n * (double)ONE) / (double)d;
	return x;
}

void
rectf(Bitmap *b, Rectangle r, Fcode f)
{

	if(f == F_XOR)
		f = notD;
	bitblt(b, r.min, b, r, f);
}

void
ringbell(void)
{
}

int
button(int b)
{

	if(ecanmouse())
		mouse = emouse();
	return mouse.buttons & (1<<(b-1));
}

int	timef	= -1;
long
realtime(void)
{
	char buf[4*12];
	long t;

	if(timef < 0)
		timef = open("/dev/cputime", 0);
	if(timef < 0)
		exits("1");
	seek(timef, 0, 0);
	read(timef, buf, sizeof(buf));
	t = atoi(buf+2*12);
	return (t*6)/100;	/* 60th sec */
}

void
nap(void)
{
	sleep(17);
}
