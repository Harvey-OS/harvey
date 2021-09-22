#include "audio.h"

enum
{
	Ilog	= 6,
	Isiz	= 1<<Ilog,
	Nbuf	= 5209*3,	// approx 1 meg
};

typedef	struct	Jbuf	Jbuf;
struct	Jbuf
{
	long	base;
	uchar	buf[Isiz];
	Jbuf*	next;
};

typedef	struct	Ibuf	Ibuf;
struct	Ibuf
{
	int	fd;
	long	base;
	Jbuf*	buf[Nbuf/3];
};

Ibuf*
Iopen(char *f, int)
{
	int i;
	Ibuf *b;
	Jbuf *c1, *c2, *c3;

	b = malloc(sizeof(*b));
	if(b == 0)
		return 0;
	memset(b, 0, sizeof(*b));
	b->fd = open(f, OREAD);
	if(b->fd < 0)
		return 0;
	for(i=0; i<nelem(b->buf); i++) {
		c1 = malloc(sizeof(*c1));
		c2 = malloc(sizeof(*c2));
		c3 = malloc(sizeof(*c3));
		if(c1 == 0 || c2 == 0 || c3 == 0)
			return 0;
		c1->base = -1;
		c2->base = -1;
		c3->base = -1;

		b->buf[i] = c1;
		c1->next = c2;
		c2->next = c3;
		c3->next = c1;
	}
	b->base = 0;
	return b;
}

long
Iseek(Ibuf *b, long o, int p)
{
	long base;

	switch(p) {
	default:
		fprint(2, "bad seek pointed: %d\n", p);
		exits(0);
	case 0:
		base = 0;
		break;
	case 1:
		base = b->base;
		break;
	case 2:
		base = seek(b->fd, 0, 2);
	}
	base += o;
	b->base = base;
	return base;
}

Jbuf*
getbuf(Ibuf *b, long base)
{
	int h, n;
	Jbuf *c1, *c2, *c3;

	h = base % nelem(b->buf);
	if(h < 0)
		h = 0;

	/* 1st recently matched buf */
	c1 = b->buf[h];
	if(c1->base == base)
		return c1;

	/* 3rd recently matched buf */
	c3 = c1->next;
	if(c3->base == base) {
		b->buf[h] = c3;
		return c3;
	}

	/* 2nd recently matched buf */
	c2 = c3->next;
	if(c2->base == base) {
		b->buf[h] = c2;
		c2->next = c3;
		c3->next = c1;
		c1->next = c2;
		return c2;
	}

	/* overwrite 3rd recently matched buf */
	if(seek(b->fd, base, 0) != base) {
		fprint(2, "seek error: %r\n");
		exits(0);
	}
	n = read(b->fd, c3->buf, Isiz);
	if(n < Isiz) {
		if(n <= 0) {
			fprint(2, "read error: o=%ld, n=%d %r\n", base, n);
			exits(0);
		}
		memset(c3->buf+n, 0, Isiz-n);
	}
	c3->base = base;
	b->buf[h] = c3;
	return c3;
}

int
Igetc1(Ibuf *b, long base)
{
	int off;
	Jbuf *c;

	b->base = base+1;
	off = base & (Isiz-1);
	base = base & ~(Isiz-1);

	c = getbuf(b, base);
	return c->buf[off];
}

int
Igetc(Ibuf *b)
{

	return Igetc1(b, b->base);
}
