/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	src	"/sys/src/libstdio"
#pragma	lib	"libstdio.a"

/*
 * pANS astdio.h
 */
/*
 * According to X3J11, there is only one i/o buffer
 * and it must not be occupied by both input and output data.
 *	If rp<wp, we must have state==RD and
 *	if wp<rp, we must have state==WR, so that getc and putc work correctly.
 *	On open, rp, wp and buf are set to 0, so first getc or putc will call _IO_getc
 *	or _IO_putc, which will allocate the buffer.
 *	If setvbuf(., ., _IONBF, .) is called, bufl is set to 0 and
 *	buf, rp and wp are pointed at unbuf.
 *	If setvbuf(., ., _IOLBF, .) is called, _IO_putc leaves wp and rp pointed at the
 *	end of the buffer so that it can be called on each putc to check whether it's got
 *	a newline.  This nonsense is in order to avoid impacting performance of the other
 *	buffering modes more than necessary -- putting the test in putc adds many
 *	instructions that are wasted in non-_IOLBF mode:
 *	#define putc(c, f)	(_IO_ctmp=(c),\
 *				(f)->wp>=(f)->rp || (f)->flags&LINEBUF && _IO_ctmp=='\n'\
 *					?_IO_putc(_IO_ctmp, f)\
 *					:*(f)->wp++=_IO_ctmp)
 *				
 */
typedef struct{
	int fd;		/* UNIX file pointer */
	int8_t flags;	/* bits for must free buffer on close, line-buffered */
	int8_t state;	/* last operation was read, write, position, error, eof */
	int8_t *buf;	/* pointer to i/o buffer */
	int8_t *rp;	/* read pointer (or write end-of-buffer) */
	int8_t *wp;	/* write pointer (or read end-of-buffer) */
	int8_t *lp;	/* actual write pointer used when line-buffering */
	int32_t bufl;	/* actual length of buffer */
	int8_t unbuf[1];	/* tiny buffer for unbuffered io (used for ungetc?) */
}FILE;
typedef int32_t fpos_t;
#ifndef NULL
#define	NULL	((void*)0)
#endif
/*
 * Third arg of setvbuf
 */
#define	_IOFBF	1			/* block-buffered */
#define	_IOLBF	2			/* line-buffered */
#define	_IONBF	3			/* unbuffered */
#define	BUFSIZ	4096			/* size of setbuf buffer */
#define	EOF	(-1)			/* returned on end of file */
#define	FOPEN_MAX	100		/* max files open */
#define	FILENAME_MAX	BUFSIZ		/* silly filename length */
#define	L_tmpnam	20		/* sizeof "/tmp/abcdefghij9999 */
#ifndef SEEK_SET			/* also defined in unistd.h */
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	SEEK_SET	0
#endif
#define	TMP_MAX		64		/* very hard to set correctly */
#define	stderr	(&_IO_stream[2])
#define	stdin	(&_IO_stream[0])
#define	stdout	(&_IO_stream[1])
#define	_IO_CHMASK	0377		/* mask for 8 bit characters */
FILE *tmpfile(void);
int8_t *tmpnam(int8_t *);
int fclose(FILE *);
int fflush(FILE *);
FILE *fopen(const int8_t *, const int8_t *);
FILE *fdopen(const int, const int8_t *);
FILE *freopen(const int8_t *, const int8_t *, FILE *);
void setbuf(FILE *, int8_t *);
int setvbuf(FILE *, int8_t *, int, int32_t);
int fprintf(FILE *, const int8_t *, ...);
int fscanf(FILE *, const int8_t *, ...);
int printf(const int8_t *, ...);
int scanf(const int8_t *, ...);
int sprintf(int8_t *, const int8_t *, ...);
int snprintf(int8_t *, int, const int8_t *, ...);
int sscanf(const int8_t *, const int8_t *, ...);
int vfprintf(FILE *, const int8_t *, va_list);
int vprintf(const int8_t *, va_list);
int vsprintf(int8_t *, const int8_t *, va_list);
int vsnprintf(int8_t *, int, const int8_t *, va_list);
int vfscanf(FILE *, const int8_t *, va_list);
int fgetc(FILE *);
int8_t *fgets(int8_t *, int, FILE *);
int fputc(int, FILE *);
int fputs(const int8_t *, FILE *);
int getc(FILE *);
#define	getc(f)	((f)->rp>=(f)->wp?_IO_getc(f):*(f)->rp++&_IO_CHMASK)
int _IO_getc(FILE *f);
int getchar(void);
#define	getchar()	getc(stdin)
int8_t *gets(int8_t *);
int putc(int, FILE *);
#define	putc(c, f) ((f)->wp>=(f)->rp?_IO_putc(c, f):(*(f)->wp++=c)&_IO_CHMASK)
int _IO_putc(int, FILE *);
int putchar(int);
#define	putchar(c)	putc(c, stdout)
int puts(const int8_t *);
int ungetc(int, FILE *);
int32_t fread(void *, int32_t, int32_t, FILE *);
int32_t fwrite(const void *, int32_t, int32_t, FILE *);
int fgetpos(FILE *, fpos_t *);
int fseek(FILE *, int32_t, int);
int fseeko(FILE *, long long, int);
int fsetpos(FILE *, const fpos_t *);
int32_t ftell(FILE *);
long long ftello(FILE *);
void rewind(FILE *);
void clearerr(FILE *);
int feof(FILE *);
int ferror(FILE *);
void perror(const int8_t *);
extern FILE _IO_stream[FOPEN_MAX];
FILE *sopenr(const int8_t *);
FILE *sopenw(void);
int8_t *sclose(FILE *);
int fileno(FILE *);
