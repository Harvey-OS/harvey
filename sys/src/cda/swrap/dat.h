/*
 * definitions for wrap programs
 */

typedef struct Pin	Pin;
typedef struct Pinent	Pinent;
typedef struct Wbuf	Wbuf;

struct Pin
{
	int	x;
	int	y;
};

struct Pinent
{
	int	x;
	int	y;
	char	chname[16];
	int	chsize;
	char	pname[16];
};

struct Wbuf
{
	int	mark;		/* see below */
	int	slen;		/* wire length */
	Pinent	pent[2];	/* pins to be wired together */
	char	stype;		/* sig type: 'w' or 's' */
	char	sname[24];	/* signal name */
};

/* values of bits in mark */

#define LEVEL1	1
#define LEVEL2	2
#define FIRSTW	4
#define LASTW	8

extern char *	mfile;		/* serial line */
extern int	debug;
extern int	absx, absy;

extern char *	wroute;		/* prefered wire route */
extern int	rot;		/* rotation, anti-clockwise, +4 if flip board over */
extern int	xbase, ybase;	/* new origin about which rotation is done */

extern char *	wrfile;		/* wrap file */
extern Biobuf *	wr;
extern int	nlines;
extern Wbuf	wbuf;

extern Pinent *	pptr[2];	/* pins in sequence for wiring */
extern char	dirn[2];	/* direction for wire run */
