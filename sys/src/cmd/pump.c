/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* pump - copy through circular buffer */
#include <u.h>
#include <libc.h>

uint8_t*	buf;

Lock	arithlock;	/* protect 64-bit accesses: unlikely to be atomic */
uint64_t	nin;
uint64_t	nout;

uint32_t	kilo;
uint32_t	max;
int32_t	ssize;
int64_t	tsize;
int	dsize;
int	done;
int	ibsize;
int	obsize;
int	verb;
int64_t	off;

void	doinput(int);
void	dooutput(int);

static void
usage(void)
{
	fprint(2, "usage: pump [-b iando] [-d sleeptime] [-f ofile] "
		"[-i ireadsize]\n\t[-k KB-buffer] [-o owritesize] "
		"[-s start-KB] [-S seek-offset]\n\t[-t mins] [files]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i, f, fo;
	char *file;

	kilo = 5000;
	obsize = ibsize = 8*1024;
	dsize = 0;
	fo = 1;
	off = 0;

	ARGBEGIN {
	default:
		usage();
	case 'b':
		obsize = ibsize = atoi(EARGF(usage()));
		break;
	case 'd':
		dsize = atoi(EARGF(usage()));
		break;
	case 'f':
		file = EARGF(usage());
		fo = create(file, 1, 0666);
		if(fo < 0)
			sysfatal("can't create %s: %r", file);
		break;
	case 'i':
		ibsize = atoi(EARGF(usage()));
		break;
	case 'k':
		kilo = atoi(EARGF(usage()));
		break;
	case 'o':
		obsize = atoi(EARGF(usage()));
		break;
	case 's':
		ssize = atoi(EARGF(usage()));
		if(ssize <= 0)
			ssize = 800;
		ssize <<= 10;
		break;
	case 'S':
		off = atoll(EARGF(usage()));
		if(off < 0)
			sysfatal("seek offset %lld must be non-negative", off);
		break;
	case 't':
		tsize = atoll(EARGF(usage()));
		tsize *= 10584000;		/* minutes */
		break;
	} ARGEND
	kilo <<= 10;

	buf = malloc(kilo);
	if(buf == nil)
		sysfatal("no memory: %r");
	nin = 0;
	nout = 0;
	done = 0;
	max = 0;

	switch(rfork(RFPROC|RFNOWAIT|RFNAMEG|RFMEM)) {
	default:
		dooutput(fo);
		break;
	case 0:
		for(i=0; i<argc; i++) {
			f = open(argv[i], OREAD);
			if(f < 0) {
				fprint(2, "%s: can't open %s: %r\n",
					argv0, argv[i]);
				break;
			}
			doinput(f);
			close(f);
		}
		if(argc == 0)
			doinput(0);
		break;
	case -1:
		fprint(2, "%s: fork failed: %r\n", argv0);
		break;
	}
	done = 1;
	exits(0);
}

/* call with arithlock held */
static int
sleepunlocked(int32_t ms)
{
	int r;

 	unlock(&arithlock);
	r = sleep(ms);
 	lock(&arithlock);
	return r;

}

void
dooutput(int f)
{
	int32_t n, l, c;

	seek(f, off, 0);
	lock(&arithlock);
	for (;;) {
		n = nin - nout;
		if(n == 0) {
			if(done)
				break;
			sleepunlocked(dsize);
			continue;
		}
		if(verb && n > max) {
			fprint(2, "n = %ld\n", n);
			max = n;
		}
		l = nout % kilo;
		unlock(&arithlock);

		if(kilo-l < n)
			n = kilo-l;
		if(n > obsize)
			n = obsize;
		c = write(f, buf+l, n);

		lock(&arithlock);
		if(c != n) {
			fprint(2, "%s: write error at offset %,lld: %r\n",
				argv0, seek(f, 0, 1));
			break;
		}
		nout += c;
		if(tsize && nout > tsize) {
			fprint(2, "%s: time limit exceeded\n", argv0);
			break;
		}
	}
	unlock(&arithlock);
}

void
doinput(int f)
{
	int32_t n, l, c, xnin;

	seek(f, off, 0);
	lock(&arithlock);
	if(ssize > 0) {
		for (xnin = 0; xnin < ssize && !done; xnin += c) {
			n = kilo - (xnin - nout);
			if(n == 0)
				break;
			unlock(&arithlock);

			l = xnin % kilo;
			if(kilo-l < n)
				n = kilo-l;
			if(n > ibsize)
				n = ibsize;
			c = read(f, buf+l, n);

			lock(&arithlock);
			if(c <= 0) {
				if(c < 0)
					fprint(2, "%s: read error: %r\n", argv0);
				break;
			}
		}
		nin = xnin;
	}
	while(!done) {
		n = kilo - (nin - nout);
		if(n == 0) {
			sleepunlocked(0);
			continue;
		}
		l = nin % kilo;
		unlock(&arithlock);

		if(kilo-l < n)
			n = kilo-l;
		if(n > ibsize)
			n = ibsize;
		c = read(f, buf+l, n);

		lock(&arithlock);
		if(c <= 0) {
			if(c < 0)
				fprint(2, "%s: read error: %r\n", argv0);
			break;
		}
		nin += c;
	}
	unlock(&arithlock);
}
