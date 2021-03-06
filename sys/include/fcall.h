/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


#define	VERSION9P	"9P2000"

#define	MAXWELEM	16

typedef
struct	Fcall
{
	u8	type;
	u32	fid;
	u16	tag;
	union {
		struct {
			u32	msize;		/* Tversion, Rversion */
			char	*version;	/* Tversion, Rversion */
		};
		struct {
			u16	oldtag;		/* Tflush */
		};
		struct {
			char	*ename;		/* Rerror */
		};
		struct {
			u32	errno;		/* Rlerror */
		};
		struct {
			Qid	qid;		/* Rattach, Ropen, Rcreate */
			u32	iounit;		/* Ropen, Rcreate */
		};
		struct {
			Qid	aqid;		/* Rauth */
		};
		struct {
			u32	afid;		/* Tauth, Tattach */
			char	*uname;		/* Tauth, Tattach */
			char	*aname;		/* Tauth, Tattach */
		};
		struct {
			u32	perm;		/* Tcreate */ 
			char	*name;		/* Tcreate */
			u8	mode;		/* Tcreate, Topen */
		};
		struct {
			u32	newfid;		/* Twalk */
			u16	nwname;		/* Twalk */
			char	*wname[MAXWELEM];	/* Twalk */
		};
		struct {
			u16	nwqid;		/* Rwalk */
			Qid	wqid[MAXWELEM];		/* Rwalk */
		};
		struct {
			i64	offset;		/* Tread, Twrite */
			u32	count;		/* Tread, Twrite, Rread */
			char	*data;		/* Twrite, Rread */
		};
		struct {
			u16	nstat;		/* Twstat, Rstat */
			u8	*stat;		/* Twstat, Rstat */
		};
	};
} Fcall;


#define	GBIT8(p)	((p)[0])
#define	GBIT16(p)	((p)[0]|((p)[1]<<8))
#define	GBIT32(p)	((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))
#define	GBIT64(p)	((u32)((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24)) |\
				((i64)((p)[4]|((p)[5]<<8)|((p)[6]<<16)|((p)[7]<<24)) << 32))

#define	PBIT8(p,v)	(p)[0]=(v)
#define	PBIT16(p,v)	(p)[0]=(v);(p)[1]=(v)>>8
#define	PBIT32(p,v)	(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24
#define	PBIT64(p,v)	(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24;\
			(p)[4]=(v)>>32;(p)[5]=(v)>>40;(p)[6]=(v)>>48;(p)[7]=(v)>>56

#define	BIT8SZ		1
#define	BIT16SZ		2
#define	BIT32SZ		4
#define	BIT64SZ		8
#define	QIDSZ	(BIT8SZ+BIT32SZ+BIT64SZ)

/* STATFIXLEN includes leading 16-bit count */
/* The count, however, excludes itself; total size is BIT16SZ+count */
#define STATFIXLEN	(BIT16SZ+QIDSZ+5*BIT16SZ+4*BIT32SZ+1*BIT64SZ)	/* amount of fixed length data in a stat buffer */

#define	NOTAG		(u16)~0U	/* Dummy tag */
#define	NOFID		(u32)~0U	/* Dummy fid */
#define	IOHDRSZ		24	/* ample room for Twrite/Rread header (iounit) */

// 9P2000.L, for some reason, lets callers closely tune what comes from a gettattr.
// This *may* be because HPC file systems make getting at some types of metadata
// expensive. Or it may be a pointless over-optimization; we suspect the latter.
// Just ask for it all. Bandwidth is free.

#define P9_GETATTR_MODE         0x00000001ULL
#define P9_GETATTR_NLINK        0x00000002ULL
#define P9_GETATTR_UID          0x00000004ULL
#define P9_GETATTR_GID          0x00000008ULL
#define P9_GETATTR_RDEV         0x00000010ULL
#define P9_GETATTR_ATIME        0x00000020ULL
#define P9_GETATTR_MTIME        0x00000040ULL
#define P9_GETATTR_CTIME        0x00000080ULL
#define P9_GETATTR_INO          0x00000100ULL
#define P9_GETATTR_SIZE         0x00000200ULL
#define P9_GETATTR_BLOCKS       0x00000400ULL

#define P9_GETATTR_BTIME        0x00000800ULL
#define P9_GETATTR_GEN          0x00001000ULL
#define P9_GETATTR_DATA_VERSION 0x00002000ULL

#define P9_GETATTR_BASIC        0x000007ffULL /* Mask for fields up to BLOCKS */
#define P9_GETATTR_ALL          0x00003fffULL /* Mask for All fields above */

enum
{
	Tversion =	100,
	Rversion,
	Tauth =		102,
	Rauth,
	Tattach =	104,
	Rattach,
	Terror =	106,	/* illegal */
	Rerror,
	Tflush =	108,
	Rflush,
	Twalk =		110,
	Rwalk,
	Topen =		112,
	Ropen,
	Tcreate =	114,
	Rcreate,
	Tread =		116,
	Rread,
	Twrite =	118,
	Rwrite,
	Tclunk =	120,
	Rclunk,
	Tremove =	122,
	Rremove,
	Tstat =		124,
	Rstat,
	Twstat =	126,
	Rwstat,
	Tmax,
	DotLOffset = 100,
	Tlattach = 	Tattach - DotLOffset,
	Rlattach,
	Tlopen =	Topen - DotLOffset,
	Rlopen,
	Tlcreate =	Tcreate - DotLOffset,
	Rlcreate,
	Tlerror =	Terror - DotLOffset,
	Rlerror,
	Tgetattr =	Tstat - DotLOffset,
	Rgetattr,
	Treaddir = 	40,
	Rreaddir
};

u32	convM2S(u8*, u32, Fcall*);
u32	convS2M(Fcall*, u8*, u32);
u32	sizeS2M(Fcall*);

int	statcheck(u8 *abuf, u32 nbuf);
u32	convM2D(u8*, u32, Dir*, char*);
u32	convLM2D(u8*, u32, Dir*);
u32	convD2M(Dir*, u8*, u32);
u32	sizeD2M(Dir*);

int	fcallfmt(Fmt*);
int	dirfmt(Fmt*);
int	dirmodefmt(Fmt*);

int	read9pmsg(int, void*, u32);

