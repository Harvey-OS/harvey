#include	"type.h"

Angle
bearto(Nav *p)
{
	long z, dx, dy;

	dx = plane.lng - p->lng;
	dx = dx * C2 * plane.coslat;
	dy = p->lat - plane.lat;
	dy = dy * C2;
	z = iatan(dx, dy) - plane.magvar;
	z &= 2*PI - 1;
	return z;
}

long
fdist(Nav *p)
{
	long dx, dy;

	dx = plane.lng - p->lng;
	dx = dx * C2 * plane.coslat;
	dy = p->lat - plane.lat;
	dy = dy * C2;
	return ihyp(dx, dy);
}

void
convertll(long dx, long dy)
{

	plane.lng -= dx * (.001/C2) / plane.coslat;
	plane.lat += dy * (.001/C2);
	plane.coslat = cos(plane.lat * C1);
}

void
dconv(int d, char *s, int p)
{
	int f, i;
	char c[10];

	f = 0;
	if(d < 0) {
		f = 1;
		d = -d;
	}
	for(i=0; i<10; i++) {
		c[i] = d%10;
		d /= 10;
	}
	i = p;
	if(f)
		i--;
	for(i--; i > 0; i--) {
		if(c[i] != 0)
			break;
		*s++ = '0';
	}
	if(f)
		*s++ = '-';
	for(;i >= 0; i--)
		*s++ = c[i] + '0';
}

int
deg(Angle a)
{
	long d;

	a &= 2*PI - 1;
	d = (a + DEG(1)/2) / DEG(1);
	if(d >= DEG(360))
		d -= DEG(360);
	return d;
}

int
hit(int x, int y, int n)
{
	int t;

	t = plane.buty - y;
	if(t < 0 || t >= H)
		return -1;
	t = plane.butx - x;
	if(t < 0 || t >= n*W)
		return -1;
	return t/W;
}

int
mod3(int n, int a)
{

	switch(a)
	{
	case 0:
		if(n >= 900)
			n -= 900;
		else
			n += 100;
		break;

	case 1:
		if((n/10)%10 == 9)
			n -= 90;
		else
			n += 10;
		break;

	case 2:
		if(n%10 == 9)
			n -= 9;
		else
			n += 1;
		break;
	}
	return n;
}

int
mod6(int n, int a)
{

	switch(a)
	{
	case 1:
		if(n >= 9000)
			n -= 9000;
		else
			n += 1000;
		break;

	case 2:
		if((n/100)%10 == 9)
			n -= 900;
		else
			n += 100;
		break;

	case 4:
		if((n/10)%10 == 9)
			n -= 90;
		else
			n += 10;
		break;

	case 5:
		if(n%10 == 5)
			n -= 5;
		else
			n += 5;
		break;
	}
	return(n);
}

void
dodraw(Image *i, int d, long *l, Rectangle *rp)
{
	int cx, cy;
	Point Po, Pn;
	Fract is, ic;
	Angle a;

	a = DEG(d);
	is = isin(a);
	ic = icos(a);
	cx = rp->min.x + plane.side24;
	cy = rp->min.y + plane.side24;
	for(a = 0; l[0] != NONE; l += 2) {
		if(l[0] == BREAK) {
			a = 0;
			l++;
		}
		Pn.x = cx + fmul(l[0], ic) + fmul(l[1], is);
		Pn.y = cy + fmul(l[0], is) - fmul(l[1], ic);
		if(a)
			d_segment(i, Po, Pn, LEVEL);
		a = 1;
		Po = Pn;
	}
}

void
cross(Image *i, int nx, int ny, int ox, int oy)
{
	int x, y;

	x = nx;
	if(x != NONE) {
		x += ox;
		y = oy;
		d_set(i, Rect(x-Q, y+plane.side14, x+Q-1, y+plane.side34));
	}
	y = ny;
	if(y != NONE) {
		x = ox;
		y += oy;
		d_set(i, Rect(x+plane.side14, y-Q, x+plane.side34, y+Q-1));
	}
}

void
outd(int d)
{
	char s[10];

	strcpy(s, "XXXXXX");
	dconv(d, s, 6);
	d_string(D, Pt(plane.tempx, plane.tempy), font, s);
}

void
normal(long *r, long *l)
{

	for(; *l != NONE; l++,r++) {
		*r = *l;
		if(*r != BREAK)
			*r = muldiv(*r-500, plane.side, 1000);
	}
	*r = NONE;
}

int
rchar(int c)
{

	if(c < 26)
		return('A'+c);
	c -= 26;
	if(c < 10)
		return('0'+c);
	c -= 10;
	if(c < 3)
		return "-._"[c];
	return 0;
}
