/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	size_t bufl;	/* actual length of buffer */
	int8_t unbuf[1];	/* tiny buffer for unbuffered io (used for ungetc?) */
}FILE;
typedef long long fpos_t;
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
#define	BUFSIZ	4096			/* size of setbuf buffer */
#define	EOF	(-1)			/* returned on end of file */
#define	FOPEN_MAX	90		/* max files open */
#define	FILENAME_MAX	BUFSIZ		/* silly filename length */
#define	L_tmpnam	20		/* sizeof "/tmp/abcdefghij9999 */
#define	L_cuserid	32		/* maximum size user name */
#define	L_ctermid	32		/* size of name of controlling tty */
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	SEEK_SET	0
#define	TMP_MAX		64		/* very hard to set correctly */
#define	stderr	(&_IO_stream[2])
#define	stdin	(&_IO_stream[0])
#define	stdout	(&_IO_stream[1])
#define	_IO_CHMASK	0377		/* mask for 8 bit characters */

#ifdef __cplusplus
extern "C" {
#endif

extern int remove(const int8_t *);
extern int rename(const int8_t *, const int8_t *);
extern FILE *tmpfile(void);
extern int8_t *tmpnam(int8_t *);
extern int fclose(FILE *);
extern int fflush(FILE *);
extern FILE *fopen(const int8_t *, const int8_t *);
extern FILE *freopen(const int8_t *, const int8_t *, FILE *);
extern void setbuf(FILE *, int8_t *);
extern int setvbuf(FILE *, int8_t *, int, size_t);
extern int fprintf(FILE *, const int8_t *, ...);
extern int fscanf(FILE *, const int8_t *, ...);
extern int printf(const int8_t *, ...);
extern int scanf(const int8_t *, ...);
extern int sprintf(int8_t *, const int8_t *, ...);

/*
 * NB: C99 now *requires *snprintf to return the number of characters
 * that would have been written, had there been room.
 */
extern int snprintf(int8_t *, size_t, const int8_t *, ...);
extern int vsnprintf(int8_t *, size_t, const int8_t *, va_list);

extern int sscanf(const int8_t *, const int8_t *, ...);
extern int vfprintf(FILE *, const int8_t *, va_list);
extern int vprintf(const int8_t *, va_list);
extern int vsprintf(int8_t *, const int8_t *, va_list);
extern int vfscanf(FILE *, const int8_t *, va_list);
extern int fgetc(FILE *);
extern int8_t *fgets(int8_t *, int, FILE *);
extern int fputc(int, FILE *);
extern int fputs(const int8_t *, FILE *);
extern int getc(FILE *);
#define	getc(f)	((f)->rp>=(f)->wp?_IO_getc(f):*(f)->rp++&_IO_CHMASK)
extern int _IO_getc(FILE *f);
extern int getchar(void);
#define	getchar()	getc(stdin)
extern int8_t *gets(int8_t *);
extern int putc(int, FILE *);
#define	putc(c, f) ((f)->wp>=(f)->rp?_IO_putc(c, f):(*(f)->wp++=c)&_IO_CHMASK)
extern int _IO_putc(int, FILE *);
extern int putchar(int);
#define	putchar(c)	putc(c, stdout)
extern int puts(const int8_t *);
extern int ungetc(int, FILE *);
extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
extern int fgetpos(FILE *, fpos_t *);
extern int fseek(FILE *, int32_t, int);
extern int fseeko(FILE *, off_t, int);
extern int fsetpos(FILE *, const fpos_t *);
extern int32_t ftell(FILE *);
extern off_t ftello(FILE *);
extern void rewind(FILE *);
extern void clearerr(FILE *);
extern int feof(FILE *);
extern int ferror(FILE *);
extern void perror(const int8_t *);
extern FILE _IO_stream[FOPEN_MAX];

#ifdef _POSIX_SOURCE
extern int fileno(FILE *);
extern FILE* fdopen(int, const int8_t*);
extern int8_t *ctermid(int8_t *);
#endif

#ifdef _REENTRANT_SOURCE
extern int8_t *tmpnam_r(int8_t *);
extern int8_t *ctermid_r(int8_t *);
#endif

#ifdef _BSD_EXTENSION
#pragma lib "/$M/lib/ape/libbsd.a"
extern FILE *popen(int8_t *, int8_t *);
extern int	pclose(FILE *);
#endif

#ifdef __cplusplus
}
#endif

#endif
