#ifndef	_STDIO_H_
#define	_STDIO_H_
#pragma lib "/$M/lib/ape/libap.a"

/*
 * pANS stdio.h
 */
#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>
/*
 * According to X3J11, there is only one i/o buffer
 * and it must not be occupied by both input and output data.
 *
 * If rp<wp, we must have state==RD and
 * if wp<rp, we must have state==WR, so that getc and putc work correctly.
 * On open, rp, wp and buf are set to 0, so first getc or putc will call
 * _IO_getc or _IO_putc, which will allocate the buffer.
 * If setvbuf(., ., _IONBF, .) is called, bufl is set to 0 and
 * buf, rp and wp are pointed at unbuf.
 * If setvbuf(., ., _IOLBF, .) is called, _IO_putc leaves wp and rp pointed at
 * the end of the buffer so that it can be called on each putc to check whether
 * it's got a newline.  This nonsense is in order to avoid impacting performance
 * of the other buffering modes more than necessary -- putting the test in putc
 * adds many instructions that are wasted in non-_IOLBF mode:
 * #define putc(c, f) (_IO_ctmp=(c),\
 * 		(f)->wp>=(f)->rp || (f)->flags&LINEBUF && _IO_ctmp=='\n'?\
 * 		_IO_putc(_IO_ctmp, f): *(f)->wp++=_IO_ctmp)
 */
typedef struct{
	int fd;		/* UNIX file pointer */
	char flags;	/* bits for must free buffer on close, line-buffered */
	char state;	/* last operation was read, write, position, error, eof */
	unsigned char *buf;	/* pointer to i/o buffer */
	unsigned char *rp;	/* read pointer (or write end-of-buffer) */
	unsigned char *wp;	/* write pointer (or read end-of-buffer) */
	unsigned char *lp;	/* actual write pointer used when line-buffering */
	size_t bufl;	/* actual length of buffer */
	unsigned char unbuf[1];	/* tiny buffer for unbuffered io (used for ungetc?) */
	int	junk;
}FILE;

typedef off_t fpos_t;

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

/*
 * Third arg of setvbuf
 */
#define	_IOFBF	1			/* block-buffered */
#define	_IOLBF	2			/* line-buffered */
#define	_IONBF	3			/* unbuffered */

#define	BUFSIZ	8192			/* size of setbuf buffer */
#define	EOF	(-1)			/* returned on end of file */
#define	FOPEN_MAX	128		/* max files open */
#define	FILENAME_MAX	BUFSIZ		/* silly filename length */
#define	L_tmpnam	20		/* sizeof "/tmp/abcdefghij9999 */
#define	L_cuserid	32		/* maximum size user name */
#define	L_ctermid	32		/* size of name of controlling tty */

#ifndef SEEK_SET			/* also defined in unistd.h */
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	SEEK_SET	0
#endif
#define	TMP_MAX		64		/* very hard to set correctly */

#define	stderr	(&_IO_stream[2])
#define	stdin	(&_IO_stream[0])
#define	stdout	(&_IO_stream[1])

extern FILE _IO_stream[FOPEN_MAX];

#ifdef __cplusplus
extern "C" {
#endif

int	_IO_getc(FILE *f);
int	_IO_putc(int, FILE *);
void	clearerr(FILE *);
int	fclose(FILE *);
FILE	*fdopen(const int, const char *);
int	feof(FILE *);
int	ferror(FILE *);
int	fflush(FILE *);
int	fgetc(FILE *);
int	fgetpos(FILE *, fpos_t *);
char	*fgets(char *, int, FILE *);
int	fileno(FILE *);
FILE	*fopen(const char *, const char *);
int	fprintf(FILE *, const char *, ...);
int	fputc(int, FILE *);
int	fputs(const char *, FILE *);
size_t	fread(void *, size_t, size_t, FILE *);
FILE	*freopen(const char *, const char *, FILE *);
int	fscanf(FILE *, const char *, ...);
int	fseek(FILE *, long, int);
int	fseeko(FILE *, off_t, int);
int	fsetpos(FILE *, const fpos_t *);
long	ftell(FILE *);
off_t	ftello(FILE *);
size_t	fwrite(const void *, size_t, size_t, FILE *);
int	getc(FILE *);
#define	getc(f)	((f)->rp>=(f)->wp? _IO_getc(f): *(f)->rp++)
int	getchar(void);
#define	getchar()	getc(stdin)
char	*gets(char *);
void	perror(const char *);
int	printf(const char *, ...);
int	putc(int, FILE *);
/* assignment to f->junk eliminates warnings about unused result of operation */
#define	putc(c, f) ((f)->junk = ((f)->wp>=(f)->rp? \
	_IO_putc(c, f): (*(f)->wp++ = (c))))
int	putchar(int);
#define	putchar(c)	putc(c, stdout)
int	puts(const char *);
int	remove(const char *);
int	rename(const char *, const char *);
void	rewind(FILE *);
int	scanf(const char *, ...);
void	setbuf(FILE *, char *);
int	setvbuf(FILE *, char *, int, size_t);
/*
 * NB: C99 now requires *snprintf to return the number of characters
 * that would have been written, had there been room.
 */
int	snprintf(char *, size_t, const char *, ...);
int	sprintf(char *, const char *, ...);
int	sscanf(const char *, const char *, ...);
FILE	*tmpfile(void);
char	*tmpnam(char *);
int	ungetc(int, FILE *);
int	vfprintf(FILE *, const char *, va_list);
int	vfscanf(FILE *, const char *, va_list);
int	vprintf(const char *, va_list);
int	vsnprintf(char *, size_t, const char *, va_list);
int	vsprintf(char *, const char *, va_list);

#ifdef _POSIX_SOURCE
int	fileno(FILE *);
FILE*	fdopen(int, const char*);
char	*ctermid(char *);
#endif

#ifdef _REENTRANT_SOURCE
char	*tmpnam_r(char *);
char	*ctermid_r(char *);
#endif

#ifdef _BSD_EXTENSION
#pragma lib "/$M/lib/ape/libbsd.a"
FILE	*popen(char *, char *);
int	pclose(FILE	*);
#endif

#ifdef __cplusplus
}
#endif

#endif
