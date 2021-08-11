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
Bfdopen(int fd, int mode)
{
	Biobuf *bp;

	bp = malloc(sizeof(Biobuf));
	if(bp == nil)
		return nil;
	if(Binits(bp, fd, mode, bp->b, sizeof(bp->b)) != 0) {
		free(bp);
		return nil;
	}
	bp->flag = Bmagic;			/* mark bp open & malloced */
	setmalloctag(bp, getcallerpc(&fd));
	return bp;
}

Biobuf*
Bopen(char *name, int mode)
{
	Biobuf *bp;
	int fd;

	switch(mode&~(OCEXEC|ORCLOSE|OTRUNC)) {
	default:
		fprint(2, "Bopen: unknown mode %#x\n", mode);
		return nil;
	case OREAD:
		fd = open(name, mode);
		break;
	case OWRITE:
		fd = create(name, mode, 0666);
		break;
	}
	if(fd < 0)
		return nil;
	bp = Bfdopen(fd, mode);
	if(bp == nil) {
		close(fd);
		return nil;
	}
	setmalloctag(bp, getcallerpc(&name));

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
