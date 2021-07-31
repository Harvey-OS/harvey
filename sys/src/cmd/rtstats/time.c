#include <u.h>
#include <libc.h>
#include "time.h"
#include "devrealtime.h"

Ticks	fasthz;

static uvlong
uvmuldiv(uvlong x, ulong num, ulong den)
{
	/* multiply, then divide, avoiding overflow */
	uvlong hi;

	hi = (x & 0xffffffff00000000LL) >> 32;
	x &= 0xffffffffLL;
	hi *= num;
	return (x*num + (hi%den << 32)) / den + (hi/den << 32);
}

Time
ticks2time(Ticks ticks)
{
	assert(ticks >= 0);
	return uvmuldiv(ticks, Onesecond, fasthz);
}

Ticks
time2ticks(Time time)
{
	assert(time >= 0);
	return uvmuldiv(time, fasthz, Onesecond);
}

int
timeconv(Fmt *f)
{
	char buf[128], *sign;
	Time t;
	Ticks ticks;

	buf[0] = 0;
	switch(f->r) {
	case 'U':
		ticks = va_arg(f->args, Ticks);
		t = ticks2time(ticks);
		break;
	case 'T':		// Time in nanoseconds
		t = va_arg(f->args, Time);
		break;
	default:
		return fmtstrcpy(f, "(timeconv)");
	}
	if (t < 0) {
		sign = "-";
		t = -t;
	}
	else
		sign = "";
	if (t > Onesecond)
		sprint(buf, "%s%d.%.3ds", sign, (int)(t / Onesecond), (int)(t % Onesecond)/1000000);
	else if (t > Onemillisecond)
		sprint(buf, "%s%d.%.3dms", sign, (int)(t / Onemillisecond), (int)(t % Onemillisecond)/1000);
	else if (t > Onemicrosecond)
		sprint(buf, "%s%d.%.3dµs", sign, (int)(t / Onemicrosecond), (int)(t % Onemicrosecond));
	else
		sprint(buf, "%s%dns", sign, (int)t);
	return fmtstrcpy(f, buf);
}

char *
parsetime(Time *rt, char *s)
{
	uvlong ticks;
	ulong l;
	char *e, *p;
	static int p10[] = {100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};

	if (s == nil)
		return("missing value");
	ticks=strtoul(s, &e, 10);
	if (*e == '.'){
		p = e+1;
		l = strtoul(p, &e, 10);
		if(e-p > nelem(p10))
			return "too many digits after decimal point";
		if(e-p == 0)
			return "ill-formed number";
		l *= p10[e-p-1];
	}else
		l = 0;
	if (*e == '\0' || strcmp(e, "s") == 0){
		ticks = 1000000000 * ticks + l;
	}else if (strcmp(e, "ms") == 0){
		ticks = 1000000 * ticks + l/1000;
	}else if (strcmp(e, "µs") == 0 || strcmp(e, "us") == 0){
		ticks = 1000 * ticks + l/1000000;
	}else if (strcmp(e, "ns") != 0)
		return "unrecognized unit";
	*rt = ticks;
	return nil;
}
