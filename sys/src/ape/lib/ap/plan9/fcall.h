typedef	struct	Fcall	Fcall;

/* see /sys/include/auth.h */
enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	DESKEYLEN=	7,		/* length of a des key for encrypt/decrypt */
	CHALLEN=	8,		/* length of a challenge */
	NETCHLEN=	16,		/* max network challenge length	*/
	CONFIGLEN=	14,

	KEYDBLEN=	NAMELEN+DESKEYLEN+4+2
};
#define	TICKETLEN	(CHALLEN+2*NAMELEN+DESKEYLEN+1)
#define	AUTHENTLEN	(CHALLEN+4+1)

struct	Fcall
{
	char	type;
	short	fid;
	unsigned short	tag;
	union
	{
		struct
		{
			unsigned short	oldtag;		/* T-Flush */
			Qid	qid;		/* R-Attach, R-Walk, R-Open, R-Create */
			char	rauth[AUTHENTLEN];	/* Rattach */
		};
		struct
		{
			char	uname[NAMELEN];	/* T-Attach */
			char	aname[NAMELEN];	/* T-Attach */
			char	ticket[TICKETLEN];	/* T-Attach */
			char	auth[AUTHENTLEN];/* T-Attach */
		};
		struct
		{
			char	ename[ERRLEN];	/* R-Error */
			char	authid[NAMELEN];	/* R-session */
			char	authdom[DOMLEN];	/* R-session */
			char	chal[CHALLEN];		/* T-session/R-session */
		};
		struct
		{
			long	perm;		/* T-Create */ 
			short	newfid;		/* T-Clone, T-Clwalk */
			char	name[NAMELEN];	/* T-Walk, T-Clwalk, T-Create */
			char	mode;		/* T-Create, T-Open */
		};
		struct
		{
			long	offset;		/* T-Read, T-Write */
			long	count;		/* T-Read, T-Write, R-Read */
			char	*data;		/* T-Write, R-Read */
		};
		struct
		{
			char	stat[DIRLEN];	/* T-Wstat, R-Stat */
		};
	};
};

#define	MAXFDATA	8192
#define	MAXMSG		160	/* max header sans data */
#define NOTAG		0xFFFF	/* Dummy tag */

enum
{
	Tmux =		48,
	Rmux,			/* illegal */
	Tnop =		50,
	Rnop,
	Tosession =	52,	/* illegal */
	Rosession,		/* illegal */
	Terror =	54,	/* illegal */
	Rerror,
	Tflush =	56,
	Rflush,
	Toattach =	58,	/* illegal */
	Roattach,		/* illegal */
	Tclone =	60,
	Rclone,
	Twalk =		62,
	Rwalk,
	Topen =		64,
	Ropen,
	Tcreate =	66,
	Rcreate,
	Tread =		68,
	Rread,
	Twrite =	70,
	Rwrite,
	Tclunk =	72,
	Rclunk,
	Tremove =	74,
	Rremove,
	Tstat =		76,
	Rstat,
	Twstat =	78,
	Rwstat,
	Tclwalk =	80,
	Rclwalk,
	Tauth =		82,	/* illegal */
	Rauth,			/* illegal */
	Tsession =	84,
	Rsession,
	Tattach =	86,
	Rattach,
};

int	convM2S(char*, Fcall*, int);
int	convS2M(Fcall*, char*);

int	convM2D(char*, Dir*);
int	convD2M(Dir*, char*);

char*	getS(int, char*, Fcall*, long*);
