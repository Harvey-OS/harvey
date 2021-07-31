#include <u.h>
#include <libc.h>
#include <libg.h>
#include <gnot.h>

#ifdef T386
#define LENDIAN
#endif
#ifdef Thobbit
#define LENDIAN
#endif

void
gpoint(GBitmap *b, Point p, int s, Fcode c)
{
	uchar *d;
	uchar mask;
	int l;

	if(!ptinrect(p, b->clipr))
		return;
	d = gbaddr(b, p);
	l = b->ldepth;
#ifdef LENDIAN
	mask = (~0UL)&((2<<l)-1);
	l = (p.x&(0x7>>l))<<l;
	s <<= l;
	mask <<= l;
#else
	s <<= (8-(1<<l));
	mask = (~0UL)<<(8-(1<<l));
	l = (p.x&(0x7>>l))<<l;
	s >>= l;
	mask >>= l;
#endif

	switch(c&0x1F){
	case Zero:
		*d &= ~mask;
		break;

	case DnorS:
		*d ^= (*d|~s)&mask;
		break;

	case DandnotS:
		*d ^= (*d&s)&mask;
		break;

	case notS:
		*d ^= (*d^~s)&mask;
		break;

	case notDandS:
		*d ^= (s|*d)&mask;
		break;

	case notD:
		*d ^= mask;
		break;

	case DxorS:
		*d ^= s&mask;
		break;

	case DnandS:
		*d ^= (s|~*d)&mask;
		break;

	case DandS:
		*d &= s|~mask;
		break;

	case DxnorS:
		*d ^= (~s)&mask;
		break;

	case D:
		break;

	case DornotS:
		*d |= (~s)&mask;
		break;

	case S:
		*d ^= (s^*d)&mask;
		break;

	case notDorS:
		*d ^= (~(*d&s))&mask;
		break;

	case DorS:
		*d |= s&mask;
		break;

	case F:
		*d |= mask;
		break;
	}
}
