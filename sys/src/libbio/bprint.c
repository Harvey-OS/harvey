#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int	printcol;

int
Bprint(Biobufhdr *bp, char *fmt, ...)
{
	va_list arg;
	int n, pcol;
	char *ip, *ep, *out;

	va_start(arg, fmt);
	ep = (char*)bp->ebuf;
	ip = ep + bp->ocount;
	pcol = printcol;
	out = doprint(ip, ep, fmt, arg);
	if(out >= ep-UTFmax-1) {
		Bflush(bp);
		ip = ep + bp->ocount;
		printcol = pcol;
		out = doprint(ip, ep, fmt, arg);
		if(out >= ep-UTFmax-1) {
			va_end(arg);
			return Beof;
		}
	}
	n = out-ip;
	bp->ocount += n;
	va_end(arg);
	return n;
}
