#ifndef _9FS_9P_H_
#define _9FS_9P_H_


#define U9FS_AUTHLEN 13
#define U9FS_NAMELEN    28
#define U9FS_TICKETLEN  72
#define U9FS_ERRLEN     64
#define U9FS_DOMLEN     48
#define U9FS_CHALLEN    8
#define U9FS_DIRLEN     116
#define U9FS_MAXFDATA  8192
#define U9FS_MAXDDATA  (((int)U9FS_MAXFDATA/U9FS_DIRLEN)*U9FS_DIRLEN)

#define U9P_MODE_RD    0x0
#define U9P_MODE_WR    0x1
#define U9P_MODE_RDWR  0x2
#define U9P_MODE_EX    0x3
#define U9P_MODE_TRUNC 0x10
#define U9P_MODE_CLOSE 0x40

#define U9P_PERM_CHDIR(m) (0x80000000&(m))
#define U9P_PERM_OWNER(m) ((m)&0x7)
#define U9P_PERM_GROUP(m) (((m)>>3)&0x7)
#define U9P_PERM_OTHER(m) (((m)>>6)&0x7)
#define U9P_PERM_ALL(m)   ((m)&0777)

/* this is too small */
typedef u_int32_t u9fsfh_t;

struct u9fd_qid {
	u9fsfh_t	path;
	u_int32_t	vers;
};

struct	u9fsreq {
  TAILQ_ENTRY(u9fsreq) r_chain;
  struct u9fsreq * r_rep;
  struct mbuf * r_mrep;
  struct proc	*r_procp;	/* Proc that did I/O system call */
  struct u9fsmount *r_nmp;

  /* actual content of the 9P message */
	char	r_type;
	short	r_fid;
	u_short	r_tag;
	union {
		struct {
			u_short	oldtag;		/* Tflush */
			struct u9fd_qid qid;		/* Rattach, Rwalk, Ropen, Rcreate */
			char	rauth[U9FS_AUTHLEN];	/* Rattach */
		} u1;
		struct {
			char	uname[U9FS_NAMELEN];		/* Tattach */
			char	aname[U9FS_NAMELEN];		/* Tattach */
			char	ticket[U9FS_TICKETLEN];	/* Tattach */
			char	auth[U9FS_AUTHLEN];	/* Tattach */
		} u2;
		struct {
			char	ename[U9FS_ERRLEN];		/* Rerror */
			char	authid[U9FS_NAMELEN];	/* Rsession */
			char	authdom[U9FS_DOMLEN];	/* Rsession */
			char	chal[U9FS_CHALLEN];		/* Tsession/Rsession */
		} u3;
		struct {
			u_int32_t	perm;		/* Tcreate */ 
			short	newfid;		/* Tclone, Tclwalk */
			char	name[U9FS_NAMELEN];	/* Twalk, Tclwalk, Tcreate */
			char	mode;		/* Tcreate, Topen */
		} u4;
		struct {
			u_int64_t	offset;		/* Tread, Twrite */
			u_short	        count;		/* Tread, Twrite, Rread */
			char	*data;		/* Twrite, Rread */
		} u5;
			char	stat[U9FS_DIRLEN];	/* Twstat, Rstat */
	} u;
};

#define r_oldtag u.u1.oldtag
#define r_qid u.u1.qid
#define r_rauth u.u1.rauth
#define r_uname u.u2.uname
#define r_aname u.u2.aname
#define r_ticket  u.u2.ticket
#define r_auth  u.u2.auth
#define r_ename  u.u3.ename
#define r_authid  u.u3.authid
#define r_authdom  u.u3.authdom
#define r_chal  u.u3.chal
#define r_perm  u.u4.perm
#define r_newfid  u.u4.newfid
#define r_name  u.u4.name
#define r_mode  u.u4.mode
#define r_offset  u.u5.offset
#define r_count  u.u5.count
#define r_data  u.u5.data
#define r_stat  u.stat

extern TAILQ_HEAD(u9fs_reqq, u9fsreq) u9fs_reqq;

struct u9fsdir {
  char	dir_name[U9FS_NAMELEN];
  char	dir_uid[U9FS_NAMELEN];
  char	dir_gid[U9FS_NAMELEN];
  struct u9fd_qid	dir_qid;
  u_int32_t	dir_mode;
  u_int32_t	dir_atime;
  u_int32_t	dir_mtime;
  union {
    u_int64_t	length;
    struct {	/* little endian */
      u_int32_t	llength;
      u_int32_t	hlength;
    } l;
  } u;
  u_short	dir_type;
  u_short	dir_dev;
};

#define dir_length u.length
#define dir_llength u.l.llength
#define dir_hlength u.l.hlength

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
	Ttunnel =	88,
	Rtunnel,
	Tmax
};

static char * u9p_types[] = {
  "Tnop",
  "Rnop",
  "Tosession",
  "Rosession",
  "Terror",
  "Rerror",
  "Tflush",
  "Rflush",
  "Toattach",
  "Roattach",
  "Tclone",
  "Rclone",
  "Twalk",
  "Rwalk",
  "Topen",
  "Ropen",
  "Tcreate",
  "Rcreate",
  "Tread",
  "Rread",
  "Twrite",
  "Rwrite",
  "Tclunk",
  "Rclunk",
  "Tremove",
  "Rremove",
  "Tstat",
  "Rstat",
  "Twstat",
  "Rwstat",
  "Tclwalk",
  "Rclwalk",
  "Tauth",
  "Rauth",
  "Tsession",
  "Rsession",
  "Tattach",
  "Rattach",
  "Ttunnel",
  "Rtunnel",
  "Tmax"
};

int u9p_m2s __P((char *ap, int n, struct u9fsreq *f));
int u9p_s2m __P((struct u9fsreq *f, char *ap, int copydata));
int u9p_m2d __P((char *ap, struct u9fsdir *f));
int u9p_d2m __P((struct u9fsdir *f, char *ap));
int u9p_type __P((char * t));

int u9p_m_m2s __P((struct mbuf **m, struct u9fsreq *f));
struct mbuf * u9p_m_s2m __P((struct u9fsreq *f));
int u9p_m_m2d __P((struct mbuf **m, struct u9fsdir *f));
struct mbuf * u9p_m_d2m __P((struct u9fsdir *f));
u_short u9p_m_tag __P((struct mbuf **m));

#endif
