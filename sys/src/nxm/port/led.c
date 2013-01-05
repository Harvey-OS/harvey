#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "../port/error.h"
#include "fns.h"
#include "led.h"

static char *ibpinames[Ibpilast] = {
[Ibpinone]	"none",
[Ibpinormal]	"normal",
[Ibpilocate]	"locate",
[Ibpifail]		"fail",
[Ibpirebuild]	"rebuild",
[Ibpipfa]		"pfa",
[Ibpispare]	"spare",
[Ibpicritarray]	"critarray",
[Ibpifailarray]	"failarray",
};

char*
ledname(int c)
{
	if(c >= 0 && c < Ibpilast)
		return ibpinames[c];
	return "bad index";
}

 int
name2led(char *s)
{
	int i;

	for(i = 0; i < nelem(ibpinames); i++)
		if(strcmp(ibpinames[i], s) == 0)
			return i;
	return -1;
}

long
ledr(Ledport *p, Chan*, void *a, long n, vlong off)
{
	char buf[64];

	snprint(buf, sizeof buf, "%s\n", ledname(p->led));
	return readstr(off, a, n, buf);
}

long
ledw(Ledport *p, Chan*, void *a, long n, vlong)
{
	int i;
	Cmdbuf *cb;

	cb = parsecmd(a, n);
	i = name2led(cb->f[0]);
	free(cb);
	if(i == -1)
		error(Ebadarg);
	p->led = i;
	return n;
}
