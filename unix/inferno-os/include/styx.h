#pragma src "/usr/inferno/lib9"
#pragma	lib	"libc.a"

#define	VERSION9P	"9P2000"

#define	MAXWELEM	16

typedef
struct	Fcall
{
	uchar	type;
	u32int	fid;
	ushort	tag;
	/* union { */
		/* struct { */
			u32int	msize;		/* Tversion, Rversion */
			char	*version;	/* Tversion, Rversion */
		/* }; */
		/* struct { */
			ushort	oldtag;		/* Tflush */
		/* }; */
		/* struct { */
			char	*ename;		/* Rerror */
		/* }; */
		/* struct { */
			Qid	qid;		/* Rattach, Ropen, Rcreate */
			u32int	iounit;		/* Ropen, Rcreate */
		/* }; */
		/* struct { */
			Qid	aqid;		/* Rauth */
		/* }; */
		/* struct { */
			u32int	afid;		/* Tauth, Tattach */
			char	*uname;		/* Tauth, Tattach */
			char	*aname;		/* Tauth, Tattach */
		/* }; */
		/* struct { */
			u32int	perm;		/* Tcreate */ 
			char	*name;		/* Tcreate */
			uchar	mode;		/* Tcreate, Topen */
		/* }; */
		/* struct { */
			u32int	newfid;		/* Twalk */
			ushort	nwname;		/* Twalk */
			char	*wname[MAXWELEM];	/* Twalk */
		/* }; */
		/* struct { */
			ushort	nwqid;		/* Rwalk */
			Qid	wqid[MAXWELEM];		/* Rwalk */
		/* }; */
		/* struct { */
			vlong	offset;		/* Tread, Twrite */
			u32int	count;		/* Tread, Twrite, Rread */
			char	*data;		/* Twrite, Rread */
		/* }; */
		/* struct { */
			ushort	nstat;		/* Twstat, Rstat */
			uchar	*stat;		/* Twstat, Rstat */
		/* }; */
	/* }; */
} Fcall;


#define	GBIT8(p)	((p)[0])
#define	GBIT16(p)	((p)[0]|((p)[1]<<8))
#define	GBIT32(p)	((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))
#define	GBIT64(p)	((vlong)((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24)) |\
				((vlong)((p)[4]|((p)[5]<<8)|((p)[6]<<16)|((p)[7]<<24)) << 32))

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

#define	NOTAG		(ushort)~0U	/* Dummy tag */
#define	NOFID		(u32int)~0U	/* Dummy fid */
#define	IOHDRSZ		24	/* ample room for Twrite/Rread header (iounit) */

enum
{
	Tversion =	100,
	Rversion,
	Tauth =	102,
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
};

uint	convM2S(uchar*, uint, Fcall*);
uint	convS2M(Fcall*, uchar*, uint);
uint	sizeS2M(Fcall*);

int	statcheck(uchar *abuf, uint nbuf);
uint	convM2D(uchar*, uint, Dir*, char*);
uint	convD2M(Dir*, uchar*, uint);
uint	sizeD2M(Dir*);

int	fcallfmt(Fmt*);
int	dirfmt(Fmt*);
int	dirmodefmt(Fmt*);

int	read9pmsg(int, void*, uint);

#pragma	varargck	type	"F"	Fcall*
#pragma	varargck	type	"M"	ulong
#pragma	varargck	type	"D"	Dir*
