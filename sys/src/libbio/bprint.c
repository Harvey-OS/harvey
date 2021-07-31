#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#define	DOTDOT	(&fmt+1)
int	printcol;

int
Bprint(Biobufhdr *bp, char *fmt, ...)
{
	char *ip, *ep, *out;
	int n, pcol;

	ep = (char*)bp->ebuf;
	ip = ep + bp->ocount;
	pcol = printcol;
	out = doprint(ip, ep, fmt, DOTDOT);
	if(out >= ep-5) {
		Bflush(bp);
		ip = ep + bp->ocount;
		printcol = pcol;
		out = doprint(ip, ep, fmt, DOTDOT);
		if(out >= ep-5)
			return Beof;
	}
	n = out-ip;
	bp->ocount += n;
	return n;
}
