#pragma	lib	"libc.a"

typedef	struct	Fcall	Fcall;

struct	Fcall
{
	char	type;
	short	fid;
	ushort	tag;
	union {
		struct {
			ushort	oldtag;		/* Tflush */
			Qid	qid;		/* Rattach, Rwalk, Ropen, Rcreate */
		};
		struct {
			char	uname[NAMELEN];	/* Tauth, Tattach */
			char	aname[NAMELEN];	/* Tattach */
			char	auth[NAMELEN];	/* Tattach */
			char	chal[8+NAMELEN];/* Tauth, Rauth */
		};
		struct {
			char	ename[ERRLEN];	/* Rerror */
		};
		struct {
			long	perm;		/* Tcreate */ 
			short	newfid;		/* Tclone, Tclwalk */
			char	name[NAMELEN];	/* Twalk, Tclwalk, Tcreate */
			char	mode;		/* Tcreate, Topen */
		};
		struct {
			long	offset;		/* Tread, Twrite */
			long	count;		/* Tread, Twrite, Rread */
			char	*data;		/* Twrite, Rread */
		};
		struct {
			char	stat[DIRLEN];	/* Twstat, Rstat */
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

int	fcallconv(void *, Fconv*);
int	dirconv(void *, Fconv*);
int	dirmodeconv(void *, Fconv*);

char*	getS(int, char*, Fcall*, long*);
