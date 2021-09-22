typedef struct Fref Fref;
typedef struct Fcp Fcp;
typedef struct Am Am;

enum{
	MaxPath = 20,

	/* types of fref */
	IsRoot	= 1 << 0,
	IsShort	= 1 << 1,
	IsLong	= 1 << 2,
	IsPath	= 1 << 3,
	IsRel		= 1 << 4,
	IsQual	= 1<< 5,


	Fdshare = 1<< 6,


	/*	The msk are ones and the other
	 *	are after masking to
	 *	compare with == . Sigh
	 */	
	
	FdKindMsk = 0x38,
	FdWorkEf = 0x00,
	FdInternEf = 0x10,
	
	FdObjMsk = 0x3f,
	FdDf = 0x38,
	FdBerTlv = 0x39,
	FdSimpTlv = 0x3a,

	FdEfMsk = 0x07,
	FdEfNoinfo = 0x00,
	FdEfTrans = 0x01,
	FdEfLinFix = 0x02,
	FdEfLinTlvFix = 0x03,
	FdEfLinVar	= 0x04,
	FdEfLinTlvVar= 0x05,
	FdEfCyclic = 0x06,
	FdEfCyclicTlv = 0x07,


	LcsStartMsk	= 0x3,
	LcsCreat	= 0x1,
	LcsInit	= 0x3,


	LcsActMsk	= 0x5,
	LcsAct	= 0x5,
	LcsDeact	= 0x4,

	LcsTermMsk	= 0xc,
	LcsTerm	= 0xc,

	LcsPropMsk	= 0xf0, /* Any != 0, propietary */

};

/* 
 *	paths are always absolute for us here without MF
 */
struct Fref{
	int current;
	int isdir;

	/* The file reference itself */
	ulong	type;
	uchar	shortid;
	ushort	longid;
	ushort	path[MaxPath]; 
	int	pathlen;
	uchar p1;
};

/*
 *	We call the  lower three bits Read, Modify Write,
 *	Their real meaning for DF, EF, Data objects and Tables/Views
 *	respectively:
 *
 *	DelChild ,CreaEF,CreaDF,
 *	Read , Modify, Write,
 *	Get ,	Put,	ManagSE,
 *	Fetch , UpdateDel,Insert,
 *
 *	The other four bytes for EF/DF are
 *	Deact, Act, Term,Del,
 *
 *	table and views
 *	
 *
 *
*/

enum{
	ReadSe = 0,
	ModifySe,
	WriteSe,
	
	DeactSe,
	ActSe,
	TermSe,
	DelSe,

	DropDboSe = 3,
	CreatDboSe,
	GrantRevSe,
	UserModSe,
};

enum{
	AndOp = 1,
	OrOp = 2,
	NotOp = 3,
	OneOp = 4,
	SOp = 5,

	P1Type = 1,
	P2Type = 2,
	InsType = 4,
	ClaType = 8,
};

struct Am{
	uchar comm[4];
	uchar iscomm[4];
	uchar isallowed;	/* alloif/never */
	uchar op;	/* and or not of the whole thing */
	uchar sc[8];
	uchar isauth[8];
	int	nsc;
};

/* 
 *	result of a select, file control parameter
 */
struct Fcp{
	uchar kindtag;	/* Fcp, Fmd, Fci */
	Fref f;	/* completely full */
	uchar lcs;
	ushort sefid;
	char name[64];
	uchar sc[8];
	uchar scidx[8];
	uchar nsc;
	Am	am;
};

CmdiccR *		getdata(int fd, Fref *f, uchar ins, ushort p12, uchar chan, int howmany);
int		tlv2fref(Fref *fr, Tlv *tlv);
int		fref2tlv(Tlv *tlv, Fref *fr);
CmdiccR *selectfid(int fd, Fref *f, ushort id, uchar chan, uchar p1);
void	printfcp(Fcp *fcp);
int	tlv2fcp(Fcp *fcp, uchar *buf, int bufsz);
