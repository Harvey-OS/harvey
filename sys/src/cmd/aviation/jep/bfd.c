#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#define	Maxcol	50

Biobuf*	bo;
Biobuf*	bi;
Biobuf	bio;
char	buf[1000];

int	ncol;
int	wid;
char	crec[32];
char	irec[32];
char	colsize[Maxcol];
char*	colname[Maxcol];

char*
col(int n)
{
	int i, c, lo, hi;

	lo = -1;
	hi = -1;
	for(i=0; i<n; i++) {
		c = Bgetc(bi);
		if(c < 0)
			return 0;
		if(c != ' ') {
			if(c >= 'A' && c <= 'Z')
				c += 'a'-'A';
			hi = i;
			if(lo < 0)
				lo = i;
		}
		buf[i] = c;
	}
	if(lo < 0)
		return "";
	buf[hi+1] = 0;
	return buf+lo;
}

void
main(int argc, char *argv[])
{
	int i, c;
	char *p, *file;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	} ARGEND

	file = "/n/sid/charts/stropria.bfd";
	if(argc > 0)
		file = argv[0];
	bi = Bopen(file, OREAD);
	if(bi == 0) {
		fprint(2, "cant open %s: %r\n", file);
		exits("open");
	}

	bo = &bio;
	Binit(bo, 1, OWRITE);

	for(i=0; i<nelem(irec); i++)
		irec[i] = Bgetc(bi);
	for(ncol=0;; ncol++) {
		c = Bgetc(bi);
		if(c == '\r')
			break;
		crec[0] = c;
		for(i=1; i<nelem(crec); i++)
			crec[i] = Bgetc(bi);
		crec[i] = 0;
		if(ncol >= Maxcol) {
			fprint(2, "too many cols %d\n", Maxcol);
			exits("ncol");
		}
		colsize[ncol] = crec[16];
		colname[ncol] = strdup(crec);
		wid += colsize[ncol];
//print("%2d %2d %s\n", ncol, colsize[ncol], colname[ncol]);
	}
//print("ncol = %d width = %d\n", ncol, wid);
	Bgetc(bi);

	for(i=0; i<ncol; i++) {
		p = colname[i];
		if(i > 0)
			Bprint(bo, ":%s", p);
		else
			Bprint(bo, "%s", p);
	}
	Bprint(bo, "\n");

	for(;;) {
		for(i=0; i<ncol; i++) {
			p = col(colsize[i]);
			if(p == 0)
				exits(0);
			if(i > 0)
				Bprint(bo, ":%s", p);
			else
				Bprint(bo, "%s", p);
		}
		Bgetc(bi);
		Bprint(bo, "\n");
	}
}
