typedef	struct	Fcall	Fcall;

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
		};
		struct
		{
			char	uname[NAMELEN];	/* Tauth, T-Attach */
			char	aname[NAMELEN];	/* T-Attach */
			char	auth[NAMELEN];	/* T-Attach */
			char	chal[8+NAMELEN];/* T-auth, R-auth */
		};
		struct
		{
			char	ename[ERRLEN];	/* R-Error */
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
#define	MAXMSG		128	/* max header sans data */
#define NOTAG		0xFFFF	/* Dummy tag */

enum
{
	Tmux =		48,
	Rmux,			/* illegal */
	Tnop =		50,
	Rnop,
	Tsession =	52,
	Rsession,
	Terror =	54,	/* illegal */
	Rerror,
	Tflush =	56,
	Rflush,
	Tattach =	58,
	Rattach,
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
	Tauth =		82,
	Rauth,
};

int	convM2S(char*, Fcall*, int);
int	convS2M(Fcall*, char*);

int	convM2D(char*, Dir*);
int	convD2M(Dir*, char*);

int	fcallconv(void *, int, int, int, int);
int	dirconv(void *, int, int, int, int);
int	dirmodeconv(void *, int, int, int, int);

char*	getS(int, char*, Fcall*, long*);
