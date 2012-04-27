/*
 * bsplit - split big binaries (copy-less plan 9 version)
 */

#include <u.h>
#include <libc.h>
#include <authsrv.h>		/* for ANAMELEN */

enum {
	Stdin,
	Sectsiz = 512,
	Bufsiz = 256*Sectsiz,
};

/* disk address (in bytes or sectors), also type of 2nd arg. to seek */
typedef uvlong Daddr;

#define BLEN(s)	((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

typedef struct {
	uchar*	rp;		/* first unconsumed byte */
	uchar*	wp;		/* first empty byte */
	uchar*	lim;		/* 1 past the end of the buffer */
	uchar*	base;		/* start of the buffer */
} Buffer;

typedef struct {
	/* parameters */
	Daddr	maxoutsz;	/* maximum size of output file(s) */

	Daddr	fileout;	/* bytes written to the current output file */
	char	*filenm;
	int	filesz;		/* size of filenm */
	char	*prefix;
	long	filenum;
	int	outf;		/* open output file */

	Buffer	buff;
} Copy;

/* global data */
char *argv0;

/* private data */
static Copy cp = { 512*1024*1024 };	/* default maximum size */
static int debug;

static void
bufreset(Buffer *bp)
{
	bp->rp = bp->wp = bp->base;
}

static void
bufinit(Buffer *bp, uchar *block, unsigned size)
{
	bp->base = block;
	bp->lim = bp->base + size;
	bufreset(bp);
}

static int 
eopen(char *file, int mode)
{
	int fd = open(file, mode);

	if (fd < 0)
		sysfatal("can't open %s: %r", file);
	return fd;
}

static int 
ecreate(char *file, int mode)
{
	int fd = create(file, mode, 0666);

	if (fd < 0)
		sysfatal("can't create %s: %r", file);
	return fd;
}

static char *
filename(Copy *cp)
{
	return cp->filenm;
}

static int 
opennext(Copy *cp)
{
	if (cp->outf >= 0)
		sysfatal("opennext called with file open");
	snprint(cp->filenm, cp->filesz, "%s%5.5ld", cp->prefix, cp->filenum++);
	cp->outf = ecreate(cp->filenm, OWRITE);
	cp->fileout = 0;
	return cp->outf;
}

static int 
closeout(Copy *cp)
{
	if (cp->outf >= 0) {
		if (close(cp->outf) < 0)
			sysfatal("error writing %s: %r", filename(cp));
		cp->outf = -1;
		cp->fileout = 0;
	}
	return cp->outf;
}

/*
 * process - process input file
 */
static void
process(int in, char *inname)
{
	int n = 1;
	unsigned avail, tolim, wsz;
	Buffer *bp = &cp.buff;

	USED(inname);
	do {
		if (BLEN(bp) == 0) {
			if (bp->lim == bp->wp)
				bufreset(bp);
			n = read(in, bp->wp, bp->lim - bp->wp);
			if (n <= 0)
				break;
			bp->wp += n;
		}
		if (cp.outf < 0)
			opennext(&cp);

		/*
		 * write from buffer's current point to end or enough bytes to
		 * reach file-size limit.
		 */
		avail = BLEN(bp);
		tolim = cp.maxoutsz - cp.fileout;
		wsz = (tolim < avail? tolim: avail);

		/* try to write full sectors */
		if (tolim >= avail && n > 0 && wsz >= Sectsiz)
			wsz = (wsz / Sectsiz) * Sectsiz;
		if (write(cp.outf, bp->rp, wsz) != wsz)
			sysfatal("error writing %s: %r", filename(&cp));
		bp->rp += wsz;

		cp.fileout += wsz;
		if (cp.fileout >= cp.maxoutsz)
			closeout(&cp);
	} while (n > 0 || BLEN(bp) != 0);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-d][-p pfx][-s size] [file...]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, errflg = 0;
	uchar block[Bufsiz];

	cp.prefix = "bs.";
	ARGBEGIN {
	case 'd':
		debug++;
		break;
	case 'p':
		cp.prefix = EARGF(usage());
		break;
	case 's':
		cp.maxoutsz = atoll(EARGF(usage()));
		if (cp.maxoutsz < 1)
			errflg++;
		break;
	default:
		errflg++;
		break;
	} ARGEND
	if (errflg || argc < 0)
		usage();

	bufinit(&cp.buff, block, sizeof block);
	cp.outf = -1;
	cp.filesz = strlen(cp.prefix) + 2*ANAMELEN;	/* 2* is slop */
	cp.filenm = malloc(cp.filesz + 1);
	if (cp.filenm == nil)
		sysfatal("no memory: %r");

	if (argc == 0)
		process(Stdin, "/fd/0");
	else
		for (i = 0; i < argc; i++) {
			int in = eopen(argv[i], OREAD);

			process(in, argv[i]);
			close(in);
		}
	closeout(&cp);
	exits(0);
}
