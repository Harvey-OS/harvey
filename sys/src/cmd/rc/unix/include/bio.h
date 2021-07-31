#ifndef _BIOH_
#define _BIOH_ 1

#include <sys/types.h>	/* for off_t */
#include <fcntl.h>	/* for O_RDONLY, O_WRONLY */

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
	int	runesize;		/* num of bytes of last getrune */
	int	state;		/* r/w/inactive */
	int	fid;		/* open file */
	int	flag;		/* magic if malloc'ed */
	off_t	offset;		/* offset of buffer in file */
	int	bsize;		/* size of buffer */
	unsigned char*	bbuf;		/* pointer to beginning of buffer */
	unsigned char*	ebuf;		/* pointer to end of buffer */
	unsigned char*	gbuf;		/* pointer to good data in buf */
	unsigned char	b[Bungetsize+Bsize];
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
int	Binit(Biobuf*, int, int);
int	Binits(Biobuf*, int, int, unsigned char*, int);
int	Blinelen(Biobuf*);
off_t	Boffset(Biobuf*);
Biobuf*	Bopen(char*, int);
int	Bprint(Biobuf*, char*, ...);
int	Bputc(Biobuf*, int);
void*	Brdline(Biobuf*, int);
long	Bread(Biobuf*, void*, long);
off_t	Bseek(Biobuf*, off_t, int);
int	Bterm(Biobuf*);
int	Bungetc(Biobuf*);
long	Bwrite(Biobuf*, void*, long);

long	Bgetrune(Biobuf*);
int	Bputrune(Biobuf*, long);
int	Bungetrune(Biobuf*);

#endif
