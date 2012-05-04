#pragma	src	"/usr/inferno/libbio"

typedef	struct	Biobuf	Biobuf;

enum
{
	Bsize		= 8*1024,
	Bungetsize	= 4,		/* space for ungetc */
	Bmagic		= 0x314159,
	Beof		= -1,
	Bbad		= -2,

	Binactive	= 0,		/* states */
	Bractive,
	Bwactive,
	Bracteof,

	Bend
};

struct	Biobuf
{
	int	icount;		/* neg num of bytes at eob */
	int	ocount;		/* num of bytes at bob */
	int	rdline;		/* num of bytes after rdline */
	int	runesize;	/* num of bytes of last getrune */
	int	state;		/* r/w/inactive */
	int	fid;		/* open file */
	int	flag;		/* magic if malloc'ed */
	long	offset;		/* offset of buffer in file */
	int	bsize;		/* size of buffer */
	uchar*	bbuf;		/* pointer to beginning of buffer */
	uchar*	ebuf;		/* pointer to end of buffer */
	uchar*	gbuf;		/* pointer to good data in buf */
	uchar	b[Bungetsize+Bsize];
};

#define	BGETC(bp)\
	((bp)->icount?(bp)->bbuf[(bp)->bsize+(bp)->icount++]:Bgetc((bp)))
#define	BPUTC(bp,c)\
	((bp)->ocount?(bp)->bbuf[(bp)->bsize+(bp)->ocount++]=(c),0:Bputc((bp),(c)))
#define	BOFFSET(bp)\
	(((bp)->state==Bractive)?\
		(bp)->offset + (bp)->icount:\
	(((bp)->state==Bwactive)?\
		(bp)->offset + ((bp)->bsize + (bp)->ocount):\
		-1))
#define	BLINELEN(bp)\
	(bp)->rdline
#define	BFILDES(bp)\
	(bp)->fid

int	Bbuffered(Biobuf*);
int	Bfildes(Biobuf*);
int	Bflush(Biobuf*);
int	Bgetc(Biobuf*);
int	Bgetd(Biobuf*, double*);
long	Bgetrune(Biobuf*);
int	Binit(Biobuf*, int, int);
int	Binits(Biobuf*, int, int, uchar*, int);
int	Blinelen(Biobuf*);
long	Boffset(Biobuf*);
Biobuf*	Bopen(char*, int);
int	Bprint(Biobuf*, char*, ...);
int	Bputc(Biobuf*, int);
int	Bputrune(Biobuf*, long);
void*	Brdline(Biobuf*, int);
long	Bread(Biobuf*, void*, long);
long	Bseek(Biobuf*, long, int);
int	Bterm(Biobuf*);
int	Bungetc(Biobuf*);
int	Bungetrune(Biobuf*);
long	Bwrite(Biobuf*, void*, long);

#pragma	varargck	argpos	Bprint	2
