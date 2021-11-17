#include <authsrv.h>

enum {
	DIRREC	= 116,		/* size of a directory ascii record */
	ERRREC	= 64,		/* size of a error record */
};

typedef	struct	Fcall	Fcall;

struct	Fcall
{
	char	type;
	ushort	fid;
	short	err;
	short	tag;
	union
	{
		struct
		{
			short	uid;		/* T-Userstr [obs.] */
			short	oldtag;		/* T-nFlush */
			Qid9p1	qid;		/* R-Attach, R-Clwalk, R-Walk,
						 * R-Open, R-Create */
			char	rauth[AUTHENTLEN];	/* R-attach */
		};
		struct
		{
			char	uname[NAMELEN];	/* T-nAttach */
			char	aname[NAMELEN];	/* T-nAttach */
			char	ticket[TICKETLEN];	/* T-attach */
			char	auth[AUTHENTLEN];	/* T-attach */
		};
		struct
		{
			char	ename[ERRREC];	/* R-nError */
			char	chal[CHALLEN];	/* T-session, R-session */
			char	authid[NAMELEN];	/* R-session */
			char	authdom[DOMLEN];	/* R-session */
		};
		struct
		{
			char	name[NAMELEN];	/* T-Walk, T-Clwalk, T-Create, T-Remove */
			long	perm;		/* T-Create */
			ushort	newfid;		/* T-Clone, T-Clwalk */
			char	mode;		/* T-Create, T-Open */
		};
		struct
		{
			Off	offset;		/* T-Read, T-Write */
			long	count;		/* T-Read, T-Write, R-Read */
			char*	data;		/* T-Write, R-Read */
		};
		struct
		{
			char	stat[DIRREC];	/* T-Wstat, R-Stat */
		};
	};
};

/*
 * P9 protocol message types
 */
enum
{
	Tnop =		50,
	Rnop,
	Tosession =	52,
	Rosession,
	Terror =	54,	/* illegal */
	Rerror,
	Tflush =	56,
	Rflush,
	Toattach =	58,
	Roattach,
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

	MAXSYSCALL
};

int	convA2M9p1(Authenticator*, char*, char*);
void	convM2A9p1(char*, Authenticator*, char*);
void	convM2T9p1(char*, Ticket*, char*);
int	convD2M9p1(Dentry*, char*);
int	convM2D9p1(char*, Dentry*);
int	convM2S9p1(uchar*, Fcall*, int);
int	convS2M9p1(Fcall*, uchar*);
void	fcall9p1(Chan*, Fcall*, Fcall*);

void	(*call9p1[MAXSYSCALL])(Chan*, Fcall*, Fcall*);
