#pragma	src	"/sys/src/libbio"
#pragma	lib	"libbio.a"

typedef	struct	Biobuf	Biobuf;
typedef	struct	Biobufhdr	Biobufhdr;

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
};

struct	Biobufhdr
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
};

struct	Biobuf
{
	Biobufhdr;
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

int	Bbuffered(Biobufhdr*);
int	Bfildes(Biobufhdr*);
int	Bflush(Biobufhdr*);
int	Bgetc(Biobufhdr*);
int	Bgetd(Biobufhdr*, double*);
long	Bgetrune(Biobufhdr*);
int	Binit(Biobuf*, int, int);
int	Binits(Biobufhdr*, int, int, uchar*, int);
int	Blinelen(Biobufhdr*);
long	Boffset(Biobufhdr*);
Biobuf*	Bopen(char*, int);
int	Bprint(Biobufhdr*, char*, ...);
int	Bputc(Biobufhdr*, int);
int	Bputrune(Biobufhdr*, long);
void*	Brdline(Biobufhdr*, int);
long	Bread(Biobufhdr*, void*, long);
long	Bseek(Biobufhdr*, long, int);
int	Bterm(Biobufhdr*);
int	Bungetc(Biobufhdr*);
int	Bungetrune(Biobufhdr*);
long	Bwrite(Biobufhdr*, void*, long);
