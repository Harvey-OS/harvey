aggr Fcall
{
	char	type;
	sint	fid;
	usint	tag;
	union
	{
		aggr
		{
			usint	oldtag;		/* T-Flush */
			Qid	qid;		/* R-Attach, R-Walk, R-Open, R-Create */
		};
		aggr
		{
			char	uname[NAMELEN];	/* Tauth, T-Attach */
			char	aname[NAMELEN];	/* T-Attach */
			char	auth[NAMELEN];	/* T-Attach */
			char	chal[8+NAMELEN];/* T-auth, R-auth */
		};
		aggr
		{
			char	ename[ERRLEN];	/* R-Error */
		};
		aggr
		{
			uint	perm;		/* T-Create */ 
			sint	newfid;		/* T-Clone, T-Clwalk */
			char	name[NAMELEN];	/* T-Walk, T-Clwalk, T-Create */
			char	mode;		/* T-Create, T-Open */
		};
		aggr
		{
			int	offset;		/* T-Read, T-Write */
			int	count;		/* T-Read, T-Write, R-Read */
			char	*data;		/* T-Write, R-Read */
		};
		aggr
		{
			char	stat[DIRLEN];	/* T-Wstat, R-Stat */
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

int	fcallconv(Printspec*);
int	dirconv(Printspec*);
int	dirmodeconv(Printspec*);
