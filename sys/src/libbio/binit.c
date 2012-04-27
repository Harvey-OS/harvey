#include	<u.h>
#include	<libc.h>
#include	<bio.h>

static	Biobufhdr*	wbufs[20];
static	int		atexitflag;

static
void
batexit(void)
{
	Biobufhdr *bp;
	int i;

	for(i=0; i<nelem(wbufs); i++) {
		bp = wbufs[i];
		if(bp != 0) {
			wbufs[i] = 0;
			Bflush(bp);
		}
	}
}

static
void
deinstall(Biobufhdr *bp)
{
	int i;

	for(i=0; i<nelem(wbufs); i++)
		if(wbufs[i] == bp)
			wbufs[i] = 0;
}

static
void
install(Biobufhdr *bp)
{
	int i;

	deinstall(bp);
	for(i=0; i<nelem(wbufs); i++)
		if(wbufs[i] == 0) {
			wbufs[i] = bp;
			break;
		}
	if(atexitflag == 0) {
		atexitflag = 1;
		atexit(batexit);
	}
}

int
Binits(Biobufhdr *bp, int f, int mode, uchar *p, int size)
{

	p += Bungetsize;	/* make room for Bungets */
	size -= Bungetsize;

	switch(mode&~(OCEXEC|ORCLOSE|OTRUNC)) {
	default:
		fprint(2, "Binits: unknown mode %d\n", mode);
		return Beof;

	case OREAD:
		bp->state = Bractive;
		bp->ocount = 0;
		break;

	case OWRITE:
		install(bp);
		bp->state = Bwactive;
		bp->ocount = -size;
		break;
	}
	bp->bbuf = p;
	bp->ebuf = p+size;
	bp->bsize = size;
	bp->icount = 0;
	bp->gbuf = bp->ebuf;
	bp->fid = f;
	bp->flag = 0;
	bp->rdline = 0;
	bp->offset = 0;
	bp->runesize = 0;
	return 0;
}


int
Binit(Biobuf *bp, int f, int mode)
{
	return Binits(bp, f, mode, bp->b, sizeof(bp->b));
}

Biobuf*
Bopen(char *name, int mode)
{
	Biobuf *bp;
	int f;

	switch(mode&~(OCEXEC|ORCLOSE|OTRUNC)) {
	default:
		fprint(2, "Bopen: unknown mode %#x\n", mode);
		return 0;
	case OREAD:
		f = open(name, mode);
		break;
	case OWRITE:
		f = create(name, mode, 0666);
		break;
	}
	if(f < 0)
		return 0;
	bp = malloc(sizeof(Biobuf));
	Binits(bp, f, mode, bp->b, sizeof(bp->b));
	bp->flag = Bmagic;			/* mark bp open & malloced */
	return bp;
}

int
Bterm(Biobufhdr *bp)
{
	int r;

	deinstall(bp);
	r = Bflush(bp);
	if(bp->flag == Bmagic) {
		bp->flag = 0;
		close(bp->fid);
		bp->fid = -1;			/* prevent accidents */
		free(bp);
	}
	/* otherwise opened with Binit(s) */
	return r;
}
