typedef
struct	Fcall
{
	uchar	type;
	short	fid;
	ushort	tag;
	ushort	oldtag;		/* Tflush */
	Qid	qid;		/* Rattach, Rwalk, Ropen, Rcreate */
	char	rauth[AUTHENTLEN];	/* Rattach */
	char	uname[NAMELEN];		/* Tattach */
	char	aname[NAMELEN];		/* Tattach */
	char	ticket[TICKETLEN];	/* Tattach */
	char	auth[AUTHENTLEN];	/* Tattach */
	char	ename[ERRLEN];		/* Rerror */
	char	authid[NAMELEN];	/* Rsession */
	char	authdom[DOMLEN];	/* Rsession */
	char	chal[CHALLEN];		/* Tsession/Rsession */
	long	perm;		/* Tcreate */ 
	short	newfid;		/* Tclone, Tclwalk */
	char	name[NAMELEN];	/* Twalk, Tclwalk, Tcreate */
	char	mode;		/* Tcreate, Topen */
	long	offset;		/* Tread, Twrite */
	long	count;		/* Tread, Twrite, Rread */
	char	*data;		/* Twrite, Rread */
	char	stat[DIRLEN];	/* Twstat, Rstat */
} Fcall;

#define	MAXMSG		160	/* max header sans data */
#define	NOTAG		0xFFFF	/* Dummy tag */

enum
{
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
	Tmax
};

int	convM2S(char*, Fcall*, int);
int	convS2M(Fcall*, char*);

int	convM2D(char*, Dir*);
int	convD2M(Dir*, char*);

int	fcallconv(va_list *, Fconv*);
int	dirconv(va_list *, Fconv*);

char*	getS(int, char*, Fcall*, long*);

