#include <u.h>
#include <libc.h>
#include <oventi.h>

/* score of a zero length block */
uchar	vtZeroScore[VtScoreSize] = {
	0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
	0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
};


int
vtZeroExtend(int type, uchar *buf, int n, int nn)
{
	uchar *p, *ep;

	switch(type) {
	default:
		memset(buf+n, 0, nn-n);
		break;
	case VtPointerType0:
	case VtPointerType1:
	case VtPointerType2:
	case VtPointerType3:
	case VtPointerType4:
	case VtPointerType5:
	case VtPointerType6:
	case VtPointerType7:
	case VtPointerType8:
	case VtPointerType9:
		p = buf + (n/VtScoreSize)*VtScoreSize;
		ep = buf + (nn/VtScoreSize)*VtScoreSize;
		while(p < ep) {
			memmove(p, vtZeroScore, VtScoreSize);
			p += VtScoreSize;
		}
		memset(p, 0, buf+nn-p);
		break;
	}
	return 1;
}

int 
vtZeroTruncate(int type, uchar *buf, int n)
{
	uchar *p;

	switch(type) {
	default:
		for(p = buf + n; p > buf; p--) {
			if(p[-1] != 0)
				break;
		}
		return p - buf;
	case VtRootType:
		if(n < VtRootSize)
			return n;
		return VtRootSize;
	case VtPointerType0:
	case VtPointerType1:
	case VtPointerType2:
	case VtPointerType3:
	case VtPointerType4:
	case VtPointerType5:
	case VtPointerType6:
	case VtPointerType7:
	case VtPointerType8:
	case VtPointerType9:
		/* ignore slop at end of block */
		p = buf + (n/VtScoreSize)*VtScoreSize;

		while(p > buf) {
			if(memcmp(p - VtScoreSize, vtZeroScore, VtScoreSize) != 0)
				break;
			p -= VtScoreSize;
		}
		return p - buf;
	}
}
