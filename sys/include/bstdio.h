/*
 * stdio emulation via bio.
 * it's imperfect.  e.g, it uses native print formats, not ANSI's.
 * it deliberately omits gets and *scanf.
 */
#pragma	src	"/sys/src/libbstdio"
#pragma	lib	"libbstdio.a"

#include <bio.h>

#ifndef NULL
#define	NULL	nil
#endif

#define	BUFSIZ		8192		/* size of setbuf buffer */
#define	EOF		Beof		/* returned on end of file */
#define FILE		Biobuf
#define	FILENAME_MAX	BUFSIZ		/* silly filename length */
#define	FOPEN_MAX	128		/* max files open */
#define	L_tmpnam	20		/* sizeof "/tmp/abcdefghij9999 */
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	SEEK_SET	0
#define	TMP_MAX		200		/* very hard to set correctly */

#define	stderr	&_bsfd2
#define	stdin	&_bsfd0
#define	stdout	&_bsfd1

extern Biobuf _bsfd0, _bsfd1, _bsfd2;
extern Biobuf *_bs_stream[FOPEN_MAX];

void	_bs_main(int argc, char **argv);
void	_bs_threadmain(int argc, char **argv);

#define main			_bs_main
#define threadmain		_bs_threadmain

#define clearerr(bp)
#define	fclose(str)		Bterm(str)
FILE	*fdopen(int, char *);
#define	feof(bp)		((bp)->state == Bracteof)
#define ferror(fp)		0
#define	fflush(bp)		Bflush(bp)
#define	fgetc(bp)		Bgetc(bp)
#define	fgetpos(bp, offp)	(*(offp) = Boffset(bp))
char	*fgets(char *, int, FILE *);
#define	fileno(bp)		Bfildes(bp)
FILE	*fopen(char *, char *);
#define	fprintf(bp, ...)	(Bprint(bp, __VA_ARGS__), Bflush(bp))
#define	fputc(c, bp);		Bputc(bp, c)
#define	fputs(s, bp)		(Bwrite(bp, s, strlen(s)), Bflush(bp))
#define fread(buf, nel, sz, bp)	Bread(bp, buf, (nel)*(sz))
FILE	*freopen(char *, char *, FILE *);
#define	fseek			Bseek
#define	fseeko			Bseek
#define	fsetpos(bp, offp)	(*(offp) = Bseek(bp, *(offp)))
#define	ftell			Boffset
#define	ftello			Boffset
#define fwrite(buf, nel, sz, bp) Bwrite(bp, buf, (nel)*(sz))
#define	getc(bp)		Bgetc(bp)
#define	getchar()		Bgetc(stdin)
#define	printf(...)		fprintf(stdout,  __VA_ARGS__)
#define	putc(c, bp)		Bputc(bp, c)
#define	putchar(c)		Bputc(stdout, c)
#define	puts(s, bp)		(Bwrite(bp, s, strlen(s)), Bflush(bp))
#define	rewind(bp)		Bseek(bp, 0, 0)
#define	sclose(bp)		Bterm(bp)
#define	setbuf(bp, buf)
#define	setvbuf(bp, buf, mode, len)
#define	snprintf		snprint
#define	sprintf			sprint
#define	ungetc(c, bp)		Bungetc(bp)
#define	vfprintf(bp, fmt, va)	(Bvprint(bp, fmt, va), Bflush(bp))
#define	vsnprintf		vsnprint
#define	vsprintf		vsprint
