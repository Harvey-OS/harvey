#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"auth.h"

#define	CHAT(cp)	((cons.flags&chatflag)||(cp&&(((Chan*)cp)->flags&chatflag)))
#define	DEV(a,b,c,d)	(Device){a,b,c,d}
#define	QID(a,b)	(Qid){a,b}
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

#define	QPDIR		0x80000000L
#define	QPNONE		0
#define	QPROOT		1
#define	QPSUPER		2

/*
 * perm argument in p9 create
 */
#define	PDIR	(1L<<31)	/* is a directory */
#define	PAPND	(1L<<30)	/* is append only */
#define	PLOCK	(1L<<29)	/* is locked on open */

#define	FID1		1
#define	FID2		2

#define	NOF	(-1)

#define SECOND(n) 	(n)
#define MINUTE(n)	(n*SECOND(60))
#define HOUR(n)		(n*MINUTE(60))
#define DAY(n)		(n*HOUR(24))
#define	MAXBIAS		SECOND(20)
#define	TLOCK		MINUTE(5)
#define	TWORM		MINUTE(2)

#define	NQUEUE		20

Nvrsafe	nvr;
Uid*	uid;
short*	gidspace;
Lock	printing;
Time	tim;
File*	files;
Wpath*	wpaths;
Lock	wpathlock;
char*	errstr[MAXERR];
void	(*p9call[MAXSYSCALL])(Chan*, Fcall*, Fcall*);
Chan*	chans;
RWlock	mainlock;
long	mktime;
long	boottime;
Queue*	serveq;
Queue*	raheadq;
Rabuf*	rabuffree;
QLock	reflock;
Lock	rabuflock;
Tlock	tlocks[NTLOCK];
Device	devnone;
long	startsb;
int	predawn;		/* set in early boot, causes polling ttyout */
int	mballocs[MAXCAT];
Filsys	filsys[10];		/* named file systems -- from config block */
char	service[50];		/* my name -- from config block */
uchar	sysip[Pasize];		/* my ip -- from config block */
uchar	defmask[Pasize];	/* ip mask -- from config block */
uchar	defgwip[Pasize];	/* gateway ip -- from config block */
uchar 	authip[Pasize];		/* ip address of server -- from config block */
char	srvkey[DESKEYLEN];
ulong	dkflag;
ulong	roflag;
ulong	chatflag;
ulong	attachflag;
int	noattach;
Ifc*	enets;			/* List of configured interfaces */
int	echo;
int	wstatallow;		/* set to circumvent wstat permissions */
int	writeallow;		/* set to circumvent write permissions */
Device	cwdevs[100];		/* structures for pseudo devices */

int	noauth;			/* Debug */
Queue*	authreply;		/* Auth replys */
Chan*	authchan;

File*	flist[5003];		/* base of file structures */
Lock	flock;			/* manipulate flist */

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
