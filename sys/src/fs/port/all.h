#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"

#define	CHAT(cp)	((cons.flags&chatflag)||(cp&&(((Chan*)cp)->flags&chatflag)))
#define	QID9P1(a,b)	(Qid9p1){a,b}
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

#define	QPDIR		0x80000000L
#define	QPNONE		0
#define	QPROOT		1
#define	QPSUPER		2

/*
 * perm argument in 9P create
 */
#define	PDIR	(1L<<31)	/* is a directory */
#define	PAPND	(1L<<30)	/* is append only */
#define	PLOCK	(1L<<29)	/* is locked on open */

#define	FID1		1
#define	FID2		2

#define SECOND(n) 	(n)
#define MINUTE(n)	(n*SECOND(60))
#define HOUR(n)		(n*MINUTE(60))
#define DAY(n)		(n*HOUR(24))
#define	MAXBIAS		SECOND(20)
#define	TLOCK		MINUTE(5)

#define	NQUEUE		20

Uid*	uid;
short*	gidspace;
Lock	printing;
Time	tim;
File*	files;
Wpath*	wpaths;
Lock	wpathlock;
char*	errstr9p[MAXERR];
Chan*	chans;
RWlock	mainlock;
ulong	mktime;
ulong	boottime;
Queue*	serveq;
Queue*	raheadq;
Rabuf*	rabuffree;
QLock	reflock;
Lock	rabuflock;
Tlock	tlocks[NTLOCK];
Lock	tlocklock;
Device*	devnone;
Startsb	startsb[5];
int	predawn;		/* set in early boot, causes polling ttyout */
int	mballocs[MAXCAT];
Filsys	filsys[10];		/* named file systems -- from config block */
char	service[50];		/* my name -- from config block */
int	aindex;

ulong	roflag;
ulong	errorflag;
ulong	chatflag;
ulong	attachflag;
ulong	authdebugflag;
ulong	authdisableflag;
int	noattach;
int	echo;
int	wstatallow;		/* set to circumvent wstat permissions */
int	writeallow;		/* set to circumvent write permissions */
int	duallow;		/* single user to allow du */

int	noauth;			/* Debug */

int	rawreadok;		/* allow reading raw data */

File*	flist[5003];		/* base of file structures */
Lock	flock;			/* manipulate flist */

long	growacct[1000];

struct
{
	RWlock	uidlock;
	Iobuf*	uidbuf;
	int	flen;
	int	find;
} uidgc;

extern	char	statecall[];
extern	char*	wormscode[];
extern	char*	tagnames[];
