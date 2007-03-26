#include <u.h>
#include <libc.h>
#include <ctype.h>
#define Tfile Tfilescsi		/* avoid name conflict */
#include <disk.h>
#undef	Tfile
#include <bio.h>
#include <ip.h>

#include "dat.h"
#include "portfns.h"

#define malloc(n)	ialloc(n, 0)

#define	CHAT(cp) ((cons.flags&chatflag) || \
			((cp) && (((Chan*)(cp))->flags&chatflag)))
#define	QID9P1(a,b)	(Qid9p1){a,b}

#define SECOND(n)	(n)
#define MINUTE(n)	((n)*SECOND(60))
#define HOUR(n)		((n)*MINUTE(60))
#define DAY(n)		((n)*HOUR(24))

enum {
	QPDIR		= 0x80000000L,
	QPNONE		= 0,
	QPROOT		= 1,
	QPSUPER		= 2,

	/*
	 * perm argument in 9P create
	 */
	PDIR		= 1L<<31,	/* is a directory */
	PAPND		= 1L<<30,	/* is append only */
	PLOCK		= 1L<<29,	/* is locked on open */

	FID1		= 1,
	FID2		= 2,

	MAXBIAS		= SECOND(20),
	TLOCK		= MINUTE(5),
};

Uid*	uid;
short*	gidspace;
Lock	printing;
Time	tim;
File*	files;
Wpath*	wpaths;
Lock	wpathlock;
char*	errstr9p[MAXERR];
Chan*	chans;
RWLock	mainlock;
Timet	fs_mktime;
Timet	boottime;
Queue*	serveq;
Queue*	raheadq;
Rabuf*	rabuffree;
QLock	reflock;
Lock	rabuflock;
Tlock	tlocks[NTLOCK];
Lock	tlocklock;
Device*	devnone;
Startsb	startsb[5];
int	mballocs[MAXCAT];

/* from config block */
char	service[50];		/* my name */
Filsys	filsys[30];		/* named file systems */
/*
 * these are only documentation, but putting them in the config block makes
 * them visible.  the real values are compiled into cwfs.
 */
typedef struct Fspar Fspar;
struct Fspar {
	char*	name;
	long	actual;		/* compiled-in value */
	long	declared;
} fspar[];

ulong	roflag;
ulong	errorflag;
ulong	chatflag;
ulong	attachflag;
ulong	authdebugflag;
ulong	authdisableflag;
int	noattach;
int	wstatallow;		/* set to circumvent wstat permissions */
int	writeallow;		/* set to circumvent write permissions */
int	duallow;		/* single user to allow du */
int	readonly;		/* disable writes if true */

int	noauth;			/* Debug */

int	rawreadok;		/* allow reading raw data */

File*	flist[5003];		/* base of file structures */
Lock	flock;			/* manipulate flist */

long	growacct[1000];

struct
{
	RWLock	uidlock;
	Iobuf*	uidbuf;
	int	flen;
	int	find;
} uidgc;

extern	char	statecall[];
extern	char*	wormscode[];
extern	char*	tagnames[];
