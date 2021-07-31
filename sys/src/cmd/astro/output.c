#include "astro.h"

void
output(char *s, Obj1 *p)
{

	if(s == 0)
		print(" SAO %5ld", sao);
	else
		print("%10s", s);
	print(" %R %D %9.4f %9.4f %9.4f",
		p->ra, p->decl2, p->az, p->el, p->semi2);
	if(s == osun.name || s == omoon.name)
		print(" %7.4f", p->mag);
	print("\n");
}

int
Rconv(void *o, Fconv *f)
{
	char s[20];
	double v;
	int h, m, c;

	v = *(double*)o;
	v = fmod(v*12/pi, 24);		/* now hours */
	h = floor(v);
	v = fmod((v-h)*60, 60);		/* now leftover minutes */
	m = floor(v);
	v = fmod((v-m)*60, 60);		/* now leftover seconds */
	c = floor(v);
	sprint(s, "%2dh%.2dm%.2ds", h, m, c);
	strconv(s, f);
	return sizeof(double);
}

int
Dconv(void *o, Fconv *f1)
{
	char s[20];
	double v;
	int h, m, c, f;

	v = *(double*)o;
	v = fmod(v/radian, 360);	/* now degrees */
	f = 0;
	if(v > 180) {
		v = 360 - v;
		f = 1;
	}
	h = floor(v);
	v = fmod((v-h)*60, 60);		/* now leftover minutes */
	m = floor(v);
	v = fmod((v-m)*60, 60);		/* now leftover seconds */
	c = floor(v);
	sprint(s, "%c%.2d°%.2d'%.2d\"", "+-"[f], h, m, c);
	strconv(s, f1);
	return sizeof(double);
}
