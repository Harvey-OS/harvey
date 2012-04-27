#include <u.h>
#include <libc.h>
#include <tube.h>
/*
 * Ten movies streaming across that, that Internet, and what happens to
 * your own personal Internet?  I just the other day got… an Internet was
 * sent by my staff at 10 o'clock in the morning on Friday.  I got it
 * yesterday [Tuesday].  Why?  Because it got tangled up with all these
 * things going on the Internet commercially.
 * 
 * […] They want to deliver vast amounts of information over the
 * Internet.  And again, the Internet is not something that you just dump
 * something on.  It's not a big truck.  It's a series of tubes.  And if
 * you don't understand, those tubes can be filled and if they are
 * filled, when you put your message in, it gets in line and it's going
 * to be delayed by anyone that puts into that tube enormous amounts of
 * material, enormous amounts of material.
 *		-- Ted Stevens, Alaskan Senator
 */

enum{Block, Dontblock, Already};	/* xsend() nb argument */

static void
coherence(void)
{
}

Tube*
newtube(ulong msz, ulong n)
{
	Tube *t;

	t = mallocz(sizeof *t + (msz+1) * n, 1);
	t->msz = msz;
	t->tsz = n;
	t->nhole = n;
	return t;
}

void
freetube(Tube *t)
{
	free(t);
}


static int
xsend(Tube *t, void *p, int nb)
{
	int n;
	uchar *c;

	assert(t != nil && p != nil);
	if(nb != Already && downsem(&t->nhole, nb) < 0)
		return -1;
	n = ainc(&t->tl) - 1;
	n %= t->tsz;
	c = (uchar*)&t[1];
	c += (1+t->msz) * n;
	memmove(c+1, p, t->msz);
	coherence();
	*c = 1;
	upsem(&t->nmsg);
	return 0;
}

static int
xrecv(Tube *t, void *p, int nb)
{
	int n;
	uchar *c;

	assert(t != nil && p != nil);
	if(nb != Already && downsem(&t->nmsg, nb) < 0)
		return -1;
	n = ainc(&t->hd) - 1;
	n %= t->tsz;
	c = (uchar*)&t[1];
	c += (1+t->msz) * n;
	while(*c == 0)
		; 		/* could yield */
	memmove(p, c+1, t->msz);
	coherence();
	*c = 0;
	upsem(&t->nhole);
	return 0;
}

void
tsend(Tube *t, void *p)
{
	xsend(t, p, Block);
}

void
trecv(Tube *t, void *p)
{
	xrecv(t, p, Block);
}

int
nbtsend(Tube *t, void *p)
{
	return xsend(t, p, Dontblock);
}

int
nbtrecv(Tube *t, void *p)
{
	return xrecv(t, p, Dontblock);
}

int
talt(Talt a[], int na)
{
	int i, n;
	int **ss;

	assert(a != nil && na > 0);
	ss = malloc(sizeof(int*) * na);
	n = 0;
	for(i = 0; i < na; i++)
		switch(a[i].op){
		case TSND:
			ss[n++] = &a[i].t->nhole;
			break;
		case TRCV:
			ss[n++] = &a[i].t->nmsg;
			break;
		case TNBSND:
			if(nbtsend(a[i].t, a[i].m) != -1)
				return i;
			break;
		case TNBRCV:
			if(nbtrecv(a[i].t, a[i].m) != -1)
				return i;
			break;
		}
	if(n == 0)
		return -1;
	i = altsems(ss, n);
	free(ss);
	if(i < 0)
		return -1;
	switch(a[i].op){
	case TSND:
		xsend(a[i].t, a[i].m, Already);
		break;
	case TRCV:
		xrecv(a[i].t, a[i].m, Already);
		break;
	default:
		sysfatal("talt");
	}
	return i;
}

