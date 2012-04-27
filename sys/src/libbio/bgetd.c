#include <u.h>
#include <libc.h>
#include <bio.h>

struct	bgetd
{
	Biobufhdr*	b;
	int		eof;
};

static int
Bgetdf(void *vp)
{
	int c;
	struct bgetd *bg = vp;

	c = Bgetc(bg->b);
	if(c == Beof)
		bg->eof = 1;
	return c;
}

int
Bgetd(Biobufhdr *bp, double *dp)
{
	double d;
	struct bgetd b;

	b.b = bp;
	b.eof = 0;
	d = charstod(Bgetdf, &b);
	if(b.eof)
		return -1;
	Bungetc(bp);
	*dp = d;
	return 1;
}
