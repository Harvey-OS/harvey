#include <u.h>
#include <libc.h>
#include <thread.h>
#include <worker.h>
#include <error.h>
#include <tos.h>

int	Reporting	= 1;
ulong	eventmask	= 0x80000000;

enum{
	Msglen = 1024,
};

int reportfd = 1;

void
reportinit(int)
{}

void
reportdiscard(void)
{}

/*
 * µs() returns a long, not a ulong, so that time stamps can be compared
 * without needing casts
 */
static long
_µs(void)
{
	static ulong multiplier, shifter;
	uvlong x;

	if(_tos->cyclefreq == 0)
		return nsec()/1000;
	if(multiplier == 0){
		shifter = (_tos->cyclefreq > 1000000000LL) ? 24 : 16;
		multiplier = (uvlong)(1000000LL << shifter) / _tos->cyclefreq;
/*
		fprint(2, "µs: multiplier %ld, cyclefreq %lld, shifter %ld\n", multiplier, _tos->cyclefreq, shifter);
 */
	}
	cycles(&x);
	return (x * multiplier) >> shifter;
}

long (*µs)(void) = _µs;

void
_report(ulong eventtype, char *fmt, ...)
{
	va_list args;
	int len;
	ulong t;
	char s[Msglen];

	if((eventtype & eventmask) == 0)
		return;
	t = µs();
	len = snprint(s, sizeof s, "%,10lud %-15.15s ",
		t, threadgetname());

	va_start(args, fmt);
	len += vsnprint(s + len, sizeof s - len, fmt, args);
	va_end(args);

	write(reportfd, s, len);
}

int
_reportcheck(ulong eventtype)
{
	return eventtype & eventmask;
}

void
_reportdata(ulong eventtype, char *name, void *pdata, int itemsize, int nitems)
{
	char buf[Msglen], *p, *ep;
	uchar *p1;
	ushort *p2;
	ulong *p4;
	uvlong *p8;
	int i;

	if((eventtype & eventmask) == 0)
		return;
	if(nitems == 0)
		return;
	p  = buf;
	ep = buf + sizeof buf;
	p1 = pdata;
	p2 = pdata;
	p4 = pdata;
	p8 = pdata;
	i = 0;
	for(;;){
		switch(itemsize){
		default:
			assert(0);
		case 1:
			p = seprint(p, ep, "%2.2ux ", *p1++);
			break;
		case 2:
			p = seprint(p, ep, "%4.4ux ", *p2++);
			break;
		case 4:
			p = seprint(p, ep, "%8.8lux ", *p4++);
			break;
		case 8:
			p = seprint(p, ep, "%16.16llux ", *p8++);
			break;
		}
		if(++i >= nitems || (i & ((0x20 >> itemsize) - 1)) == 0){
			seprint(p, ep, "\n");
			report(eventtype, "%s %s", name, buf);
			if(i >= nitems)
				break;
			p  = buf;
		}
	}
}

void
reporting(char *option)
{
	int i;

	for(i = 0; debugs[i].name; i++)
		if(!strcmp(debugs[i].name, option))
			break;

	if(debugs[i].name == nil){
		fprint(2, "Unsupported debug option %s\n", option);
		return;
	}
	if(debugs[i].val == ~0 || debugs[i].val == 0)
		eventmask = debugs[i].val;
	else
		eventmask ^= debugs[i].val;
}
