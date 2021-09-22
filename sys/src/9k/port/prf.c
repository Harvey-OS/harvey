/*
 * dumb mp-safe synchronous i8250 or sifive uart printing for last-resort
 * debugging.  not strictly portable, but the i8250 is what vendors
 * usually provide.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

enum {
	/* 8250 registers */
	Thr	= 0,
	Lsr	= 5,		/* line status */

	/* Thr bits */
	Thre	= 1<<5,		/* transmit hold register empty */

	/* sifive registers */
	Txdata	= 0,

	Notready= 1ul<<31,
};

int polledprint = 1;		/* flag: advertise availability of prf, etc. */
static Lock prlock = { 0 };

static void
prmaylock(uchar c, int dolock)
{
	ulong *uartp;
	static uchar lastc = 0;

	uartp = (ulong *)CONSREGS();
	if (uartp == nil)
		return;
	if (c == '\n' && lastc != '\r')
		prmaylock('\r', dolock);
	if (dolock)
		ilock(&prlock);
	coherence();
#ifdef SIFIVEUART
	while (uartp[Txdata] & Notready)
		pause();
	uartp[Txdata] = c;
#else
	while ((uartp[Lsr] & Thre) == 0)
		pause();
	uartp[Thr] = c;
#endif
	coherence();
	lastc = c;
	if (dolock)
		iunlock(&prlock);
}

void
pr(uchar c)
{
	prmaylock(c, 1);
}

static void
prnolock(uchar c)
{
	prmaylock(c, 0);
}

void
prn(char *s, int n)
{
	Lock *lockp;

	lockp = &prlock;
	ilock(lockp);
	while (*s != '\0' && n-- > 0)
		prnolock(*s++);
	iunlock(lockp);
}

void
prsnolock(char *s)
{
	while (*s != '\0')
		prnolock(*s++);
}

void
prs(char *s)
{
	Lock *lockp;

	lockp = &prlock;
	ilock(lockp);
	prsnolock(s);
	iunlock(lockp);
}

static char *
prnumb(char *buf, int size, va_list *argp, int ells, int base, int hassign)
{
	int sign;
	uvlong d;
	char *s;
	static char hexdigs[] = "0123456789abcdef";

	if (ells < 2)
		d = va_arg(*argp, ulong);
	else
		d = va_arg(*argp, uvlong);
	if(d == 0)
		return "0";
	s = buf+size;
	*--s = 0;

	if(hassign && (vlong)d < 0){
		d = -(vlong)d;
		sign = -1;
	}else
		sign = 1;
	for(; d > 0; d /= base)
		*--s = hexdigs[d%base];
	if(sign < 0)
		*--s = '-';
	return s;
}

/*
 * simple print for debugging
 */
int
prf(char *fmt, ...)
{
	int ells;
	uchar c;
	char *p, *s, buf[64];
	va_list arg;
	Lock *lockp;

	va_start(arg, fmt);
	lockp = &prlock;
	ilock(lockp);
	for(p = fmt; *p; p++){
		if(*p != '%') {
			prnolock(*p);
			continue;
		}
		s = "0";		/* default output */

		/* count 'l', ignore rest up to verb */
		ells = 0;
		for (; (c = p[1]) == 'l' || c == 'u' || c == '#' || c == '.' ||
		    c == ',' || c >= '0' && c <= '9'; p++)
			if (c == 'l')
				++ells;

		/* implement verbs */
		switch(*++p){
		case 'p':
		case 'P':
			ells = sizeof(uintptr) / sizeof(ulong);
			/* fall through */
		case 'x':
		case 'X':
			s = prnumb(buf, sizeof buf, &arg, ells, 16, 0);
			break;
		case 'd':
			s = prnumb(buf, sizeof buf, &arg, ells, 10, 1);
			break;
		case 's':
			s = va_arg(arg, char*);
			break;
		case '\0':
			va_end(arg);
			iunlock(lockp);
			return 0;
		}

		if (s)
			prsnolock(s);
	}
	va_end(arg);
	iunlock(lockp);
	return 0;
}
