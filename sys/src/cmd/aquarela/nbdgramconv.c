#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

int
nbdgramconvM2S(NbDgram *s, uchar *ap, uchar *ep)
{
	uchar *p = ap;
	int n;
	ushort length;

	if (ap + 6 + IPv4addrlen > ep)
		return 0;
	s->type = *p++;
	s->flags = *p++;
	s->id = nhgets(p); p+= 2;
	v4tov6(s->srcip, p); p+= IPv4addrlen;
	s->srcport = nhgets(p); p += 2;
	switch (s->type) {
	case NbDgramDirectUnique:
	case NbDgramDirectGroup:
	case NbDgramBroadcast:
		if (p + 4 > ep)
			return 0;
		length = nhgets(p); p += 2;
		s->datagram.offset = nhgets(p); p += 2;
		if (p + length > ep)
			return 0;
		ep = p + length;
		n = nbnamedecode(p, p, ep, s->datagram.srcname);
		if (n == 0)
			return 0;
		p += n;
		n = nbnamedecode(p, p, ep, s->datagram.dstname);
		if (n == 0)
			return 0;
		p += n;
		s->datagram.data = p;
		s->datagram.length = ep - p;
		p = ep;
		break;
	case NbDgramError:
		if (p + 1 > ep)
			return 0;
		s->error.code = *p++;
		break;
	case NbDgramQueryRequest:
	case NbDgramPositiveQueryResponse:
	case NbDgramNegativeQueryResponse:
		n = nbnamedecode(p, p, ep, s->query.dstname);
		if (n == 0)
			return 0;
		p += n;
		break;
	default:
		return 0;
	}
	return p - ap;
}

int
nbdgramconvS2M(uchar *ap, uchar *ep, NbDgram *s)
{
	uchar *p = ap;
	uchar *fixup;
	int n;

	if (p + 6 + IPv4addrlen > ep)
		return 0;
	*p++ = s->type;
	*p++ = s->flags;
	hnputs(p, s->id); p+= 2;
	v6tov4(p, s->srcip); p += IPv4addrlen;
	hnputs(p, s->srcport); p+= 2;
	switch (s->type) {
	case NbDgramDirectUnique:
	case NbDgramDirectGroup:
	case NbDgramBroadcast:
		if (p + 4 > ep)
			return 0;
		fixup = p;
		hnputs(p, s->datagram.length); p += 2;
		hnputs(p, s->datagram.offset); p += 2;
		n = nbnameencode(p, ep, s->datagram.srcname);
		if (n == 0)
			return 0;
		p += n;
		n = nbnameencode(p, ep, s->datagram.dstname);
		if (n == 0)
			return 0;
		p += n;
		if (p + s->datagram.length > ep)
			return 0;
		memcpy(p, s->datagram.data, s->datagram.length); p += s->datagram.length;
		hnputs(fixup, p - fixup - 4);
		break;
	case NbDgramError:
		if (p + 1 > ep)
			return 0;
		*p++ = s->error.code;
		break;
	case NbDgramQueryRequest:
	case NbDgramPositiveQueryResponse:
	case NbDgramNegativeQueryResponse:
		n = nbnameencode(p, ep, s->datagram.dstname);
		if (n == 0)
			return 0;
		p += n;
		break;
	default:
		return 0;
	}
	return p - ap;
}
