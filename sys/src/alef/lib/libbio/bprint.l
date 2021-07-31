#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.print(Biobufhdr *bp, char *fmt, ...)
{
	char *ip, *ep, *out;
	int n;

	bp->lock();
	ep = (char*)bp->ebuf;
	ip = ep + bp->ocount;
	out = doprint(ip, ep, fmt, ...);
	if(out >= ep-5) {
		bp->iflush();
		ip = ep + bp->ocount;
		out = doprint(ip, ep, fmt, ...);
		if(out >= ep-5) {
			bp->unlock();
			return Beof;
		}
	}
	n = out-ip;
	bp->ocount += n;
	bp->unlock();
	return n;
}
