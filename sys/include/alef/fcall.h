#pragma src "/sys/src/alef/lib/p9"
enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	DESKEYLEN=	7,		/* length of a des key for encrypt/decrypt */
	CHALLEN=	8,		/* length of a challenge */

	TICKETLEN=	(CHALLEN+2*NAMELEN+DESKEYLEN+1),
	AUTHENTLEN=	(CHALLEN+4+1),
};

aggr Fcall
{
	byte	type;
	sint	fid;
	usint	tag;
	union
	{
		aggr
		{
			usint	oldtag;		/* T-Flush */
			Qid	qid;		/* R-Attach, R-Walk, R-Open, R-Create */
			byte	rauth[AUTHENTLEN];	/* R-Attach */
		};
		aggr
		{
			byte	uname[NAMELEN];	/* T-Auth, T-Attach */
			byte	aname[NAMELEN];	/* T-Attach */
			byte	ticket[TICKETLEN];	/* T-Attach */
			byte	auth[AUTHENTLEN];	/* T-Attach */
		};
		aggr
		{
			byte	ename[ERRLEN];	/* R-Error */
			byte	authid[NAMELEN];	/* R-Session */
			byte	authdom[DOMLEN];	/* R-Session */
			byte	chal[CHALLEN];		/* T-Session/R-Session */
		};
		aggr
		{
			uint	perm;		/* T-Create */ 
			sint	newfid;		/* T-Clone, T-Clwalk */
			byte	name[NAMELEN];	/* T-Walk, T-Clwalk, T-Create */
			byte	mode;		/* T-Create, T-Open */
		};
		aggr
		{
			int	offset;		/* T-Read, T-Write */
			int	count;		/* T-Read, T-Write, R-Read */
			byte	*data;		/* T-Write, R-Read */
		};
		aggr
		{
			byte	stat[DIRLEN];	/* T-Wstat, R-Stat */
		};
	};
};

enum
{
	MAXFDATA =	8192,
	MAXMSG =	128,
	MAXRPC =	8192+128,
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
	Tmax,
};

int	convM2S(byte*, Fcall*, int);
int	convS2M(Fcall*, byte*);

int	convM2D(byte*, Dir*);
int	convD2M(Dir*, byte*);

int	fcallconv(Printspec*);
int	dirconv(Printspec*);
int	dirmodeconv(Printspec*);
