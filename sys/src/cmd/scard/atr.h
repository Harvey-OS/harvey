typedef struct ParsedAtr ParsedAtr;
typedef struct ParsedHist ParsedHist;
typedef struct Param Param;

struct Param{
	uchar clkrate;	/* table 6 in iso7816-3 */
	uchar bitrate;	/* table 6 in iso7816-3 */
	uchar tcck;
	uchar guardtime;
	uchar waitingint;
	/* only in T=1 */
	uchar clockstop;
	uchar ifsc;
	uchar nadvalue;
};

struct ParsedAtr{
	uchar direct;
	uchar fi;	/* freq conv factor */
	uchar di;	/* bit rate adj */
	uchar ii;	/* current */
	uchar pi;	/* voltage */
	uchar n;	/* guard time */
	uchar t;	/* t=0 block t=1 char*/
	uchar wi;	/* wait integer multiplier */
	uchar hist[32];	/* historical bytes */
	uchar nhist;	/* historical bytes */
};

enum{
	CatStat3Last = 0x00,	/* status is last 3 hist */
	CatDirref = 0x10,	/* dir reference, unknown */
	CatStatComp = 0x80,	/* status is last comp tlv */

	/* they are actually compacted so no 4 */
	TagCCountry = 0x41,
	TagCIssuerid = 0x42,	

	TagCCsrv = 0x43,		/* len always one */

	TagIniAccess = 0x44,		/* this is  in EFatr also */
	TagCIssuer = 0x45,		/* this is  in EFatr also */
	TagPreissu = 0x46,		/* unknown, manuf. info */
	TagCapab = 0x47,	

	CsrvFullDF = 1 << 7,
	CsrvPartDF = 1 << 6,
	CsrvEFdir = 1 << 5,
	CsrvEFatr = 1 << 4,

	CsrvEFPerm = (1 << 3)|(1 << 2)|(1 << 1),

	CsrvRec = 0,	/* numbers after shifting down Perm */
	CsrvGData = 2,
	CsrvBin = 4,
	
	CsrvHasMF = 1 << 0,

	CapabFullDF = 1 << 7,	/* do not confuse with Csrv, similar... */
	CapabPartDF = 1 << 6,
	CapabPath = 1 << 5,
	CapabFid = 1 << 4,
	CapabImplDF = 1 << 3,
	CapabShortEF = 1 << 2,
	CapabRecNum = 1 << 1,
	CapabRecId = 1 << 0,

	CapabTlvEF = 1 << 7,
	CapabWrite = (1 << 6)|(1 << 5),
	
	CapabWorm = 0,	/*numbers after shifting down write */
	CapabProp = 1,
	CapabWor = 2,
	CapabWand = 3,

	CapabTagFF = 1 << 4,
	CapabDUnitsz = 0xf,	/* mask */
	
	CapabCchain = 1 << 7,
	CapabExtL = 1 << 6,
	CapabRFU = 1 << 5,
	CapabCardLchan = 1 << 4,
	CapabIfcLchan = 1 << 3,
	CapabMaxLchan = 0x7,	/* mask */
	Maxpars = 8,
};

/* len 0 is not present */
struct ParsedHist{
	uchar cat;		/* category indicator, non optional */
	Tlv tlv[Maxpars];	/* indexed by TAG&0xf */
	uchar status[3];
	uchar statuslen;
	uchar csrv;
	ulong getdata;
	uchar writefun;
	uchar caplchanmax;
};


int	parseatr(ParsedAtr *pa, uchar *atr, int size);
int		parsehist(ParsedHist *pa, uchar *b, int bsz);