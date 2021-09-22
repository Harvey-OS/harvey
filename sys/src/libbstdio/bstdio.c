/*
 * stdio emulation via bio.
 * it's imperfect.  e.g, it uses native print formats, not ANSI's.
 * it deliberately omits gets and *scanf.
 */
#include <u.h>
#include <libc.h>
#include <bstdio.h>

Biobuf _bsfd0, _bsfd1, _bsfd2;

Biobuf *_bs_stream[FOPEN_MAX] = {
	&_bsfd0, &_bsfd1, &_bsfd2,
};

static int
_bstd2bmode(char *mode)
{
	if (strcmp(mode, "r") == 0)
		return OREAD;
	if (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0)
		return OWRITE;
	return ORDWR;
}

FILE *
fdopen(int fd, char *mode)
{
	FILE *fp;

	if ((uint)fd >= nelem(_bs_stream))
		return NULL;
	if (_bs_stream[fd] == NULL)
		_bs_stream[fd] = malloc(sizeof(Biobuf));
	fp = _bs_stream[fd];
	if (fp == NULL || Binit(fp, fd, _bstd2bmode(mode)) == Beof)
		return NULL;
	Bseek(fp, 0, (mode[0] == 'a'? SEEK_END: SEEK_SET));
	return fp;
}

FILE *
fopen(char *name, char *mode)
{
	FILE *fp;

	fp = Bopen(name, _bstd2bmode(mode));
	if (fp == NULL)
		return NULL;
	Bseek(fp, 0, (mode[0] == 'a'? SEEK_END: SEEK_SET));
	_bs_stream[fileno(fp)] = fp;
	return fp;
}

FILE *
freopen(char *name, char *mode, FILE *fp)
{
	FILE *nfp;

	_bs_stream[fileno(fp)] = NULL;
	Bterm(fp);
	nfp = Bopen(name, _bstd2bmode(mode));
	if (nfp == NULL)
		return NULL;
	if (nfp != fp) {			/* didn't re-use fp? */
		*fp = *nfp;
		if(nfp->flag == Bmagic) {	/* was malloced? */
			nfp->flag = 0;
			nfp->fid = -1;		/* prevent accidents */
			free(nfp);
		}
	}
	_bs_stream[fileno(fp)] = fp;
	return fp;
}

char *
fgets(char *buf, int len, FILE *bp)
{
	char *line;

	line = Brdline(bp, '\n');
	if (line == NULL)
		return NULL;
	len = Blinelen(bp);
	line[len - 1] = '\0';
	memmove(buf, line, len+1);
	return buf;
}

#ifdef notdef
#undef main
#undef threadmain

void
main(int argc, char **argv)
{
	Binit(stdin,  0, OREAD);
	Binit(stdout, 1, OWRITE);
	Binit(stderr, 2, OWRITE);
	_bs_main(argc, argv);
	exits(0);
}

void
threadmain(int argc, char **argv)
{
	Binit(stdin,  0, OREAD);
	Binit(stdout, 1, OWRITE);
	Binit(stderr, 2, OWRITE);
	_bs_threadmain(argc, argv);
	exits(0);
}
#endif
