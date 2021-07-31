#include	<alef.h>
#include	<bio.h>

#define	nelem(x)	(sizeof(x)/sizeof(x[0]))

intern	Biobufhdr*	wbufs[20];
intern	QLock		wbufl;
intern	int		atexitflag;

void
batexit(void)
{
	int i;
	Biobufhdr *bp;

	wbufl.lock();
	for(i=0; i<nelem(wbufs); i++) {
		bp = wbufs[i];
		if(bp != nil) {
			wbufs[i] = nil;
			bp->flush();
		}
	}
	wbufl.unlock();
}

intern
void
deinstall(Biobufhdr *bp)
{
	int i;

	wbufl.lock();
	for(i=0; i<nelem(wbufs); i++)
		if(wbufs[i] == bp)
			wbufs[i] = nil;
	wbufl.unlock();
}

intern
void
install(Biobufhdr *bp)
{
	int i;

	deinstall(bp);
	wbufl.lock();
	for(i=0; i<nelem(wbufs); i++)
		if(wbufs[i] == nil) {
			wbufs[i] = bp;
			break;
		}

	if(atexitflag == 0) {
		atexitflag = 1;
		atexit(batexit);
	}
	wbufl.unlock();
}

int
Biobufhdr.inits(Biobufhdr *bp, int f, int mode, char *p, int size)
{

	bp->lock();
	p += Bungetsize;	/* make room for Bungets */
	size -= Bungetsize;

	switch(mode) {
	default:
		fprint(2, "Bopen: unknown mode %d\n", mode);
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
	bp->fid = f;
	bp->flag = 0;
	bp->nrdline = 0;
	bp->off = 0;
	bp->runesize = 0;

	bp->unlock();
	return 0;
}


int
Biobuf.init(Biobuf *bp, int f, int mode)
{
	return bp->inits(f, mode, bp->b, sizeof(bp->b));
}

Biobuf*
Bopen(char *name, int mode)
{
	Biobuf *bp;
	int f;

	switch(mode) {
	default:
		fprint(2, "Bopen: unknown mode %d\n", mode);
		return nil;

	case OREAD:
		f = open(name, OREAD);
		if(f < 0)
			return nil;
		break;

	case OWRITE:
		f = create(name, OWRITE, 0666);
		if(f < 0)
			return nil;
	}
	bp = malloc(sizeof(Biobuf));
	if(bp == nil)
		return nil;
	bp->inits(f, mode, bp->b, sizeof(bp->b));
	bp->flag = Bmagic;
	return bp;
}

int
Biobufhdr.close(Biobufhdr *bp)
{
	bp->lock();
	deinstall(bp);
	bp->iflush();
	if(bp->flag == Bmagic) {
		bp->flag = 0;
		close(bp->fid);
		free(bp);
	}
	return 0;
}
