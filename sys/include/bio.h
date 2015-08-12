/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma src "/sys/src/libbio"
#pragma lib "libbio.a"

typedef struct Biobuf Biobuf;
typedef struct Biobufhdr Biobufhdr;

enum {
	Bsize = 8 * 1024,
	Bungetsize = UTFmax + 1, /* space for ungetc */
	Bmagic = 0x314159,
	Beof = -1,
	Bbad = -2,

	Binactive = 0, /* states */
	Bractive,
	Bwactive,
	Bracteof,
};

struct Biobufhdr {
	int icount;     /* neg num of bytes at eob */
	int ocount;     /* num of bytes at bob */
	int rdline;     /* num of bytes after rdline */
	int runesize;   /* num of bytes of last getrune */
	int state;      /* r/w/inactive */
	int fid;	/* open file */
	int flag;       /* magic if malloc'ed */
	int64_t offset; /* offset of buffer in file */
	int bsize;      /* size of buffer */
	uint8_t *bbuf;  /* pointer to beginning of buffer */
	uint8_t *ebuf;  /* pointer to end of buffer */
	uint8_t *gbuf;  /* pointer to good data in buf */
};

struct Biobuf {
	Biobufhdr;
	uint8_t b[Bungetsize + Bsize];
};

/* Dregs, redefined as functions for backwards compatibility */
#define BGETC(bp) Bgetc(bp)
#define BPUTC(bp, c) Bputc(bp, c)
#define BOFFSET(bp) Boffset(bp)
#define BLINELEN(bp) Blinelen(bp)
#define BFILDES(bp) Bfildes(bp)

int Bbuffered(Biobufhdr *);
int Bfildes(Biobufhdr *);
int Bflush(Biobufhdr *);
int Bgetc(Biobufhdr *);
int Bgetd(Biobufhdr *, double *);
int32_t Bgetrune(Biobufhdr *);
int Binit(Biobuf *, int, int);
int Binits(Biobufhdr *, int, int, uint8_t *, int);
int Blinelen(Biobufhdr *);
int64_t Boffset(Biobufhdr *);
Biobuf *Bopen(char *, int);
int Bprint(Biobufhdr *, char *, ...);
int Bvprint(Biobufhdr *, char *, va_list);
int Bputc(Biobufhdr *, int);
int Bputrune(Biobufhdr *, int32_t);
void *Brdline(Biobufhdr *, int);
char *Brdstr(Biobufhdr *, int, int);
int32_t Bread(Biobufhdr *, void *, int32_t);
int64_t Bseek(Biobufhdr *, int64_t, int);
int Bterm(Biobufhdr *);
int Bungetc(Biobufhdr *);
int Bungetrune(Biobufhdr *);
int32_t Bwrite(Biobufhdr *, void *, int32_t);

#pragma varargck argpos Bprint 2
