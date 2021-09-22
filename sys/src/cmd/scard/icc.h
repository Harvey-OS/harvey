#define	diprint	if(iccdebug == 0){} else fprint

extern int iccdebug;

/*
 *	As per ISO/IEC 7816-4
*/

enum{
	Maxcmd = 64,
};

typedef struct Cmdicc Cmdicc;
typedef struct CmdiccR CmdiccR;

struct Cmdicc{
	int	isext;	/* longer frames short || extended*/

	int	isspecial;	/* special not conforming packets*/
	uchar	cla;
	uchar	ins;
	uchar	p1;
	uchar	p2;
	ulong	nc;	
	uchar	*cdata;
	ulong	ne;	/*size of response? */
	uchar	cdatapack[Maxcmd];
};

struct CmdiccR{
	int		isspecial;	/* special not conforming packets*/
	uchar	*dataf;	/* at most le bytes */
	int		ndataf;
	uchar	sw1;
	uchar	sw2;
	uchar 	datafpack[Maxcmd];
};

/* INS field, standard*/
enum{
	ClaEsc = 0xff,
};

/* INS field, standard*/
enum{
	InsDeactivFile	= 	0x04,
	InsErasRecor	= 	0x0c,
	InsEraseBin	= 	0x0e,
	InsEraseBin1	= 	0x0f,
	InsScqlOp		= 	0x10,
	InsTransOp	= 	0x12,
	InsUserOp	= 	0x14,
	InsVerify		= 	0x20,
	InsVerify1		= 	0x21,
	InsChRefData	= 	0x24,
	InsDisabVerReq = 	0x26,
	InsEnabVerReq	= 	0x28,
	InsSecOp		= 	0x2a,
	InsResetBincnt	= 	0x2c,
	InsEnvel		= 	0x32,
	InsActivaFile	= 	0x44,
	InsManChan	= 	0x70,
	InsEncrypt		= 	0x74, /* ACOS specific? */
	InsExtAuth	= 	0x82,
	InsGetChall	= 	0x84,
	InsGenAuth	= 	0x86,
	InsGenAuth1	= 	0x87,
	InsIntAuth	= 	0x88,
	InsAppRecor	= 	0xE2,
	InsSearchBin	= 	0xa0,
	InsSearchbin1	= 	0xa1,
	InsSearchRecor = 	0xa2,
	InsSelect		= 	0xa4,
	InsReadBin	= 	0xb0,
	InsReadBin1	= 	0xb1,
	InsReadRec	= 	0xb2,
	InsReadRec1	= 	0xb3,
	InsGetresp	= 	0xc0,
	InsEnvel1		= 	0xc3,
	InsGetData	= 	0xca,
	InsGetData2	= 	0xcb,
	InsWriteBin	= 	0xd0,
	InsWriteRec	= 	0xd2,
	InsUpdateBin	= 	0xd6,
	InsUpdateBin1	= 	0xd7,
	InsPutData	= 	0xda,
	InsPutData1	= 	0xdb,
	InsUpdateRecor = 	0xdc,
	InsUpdateRecor1 = 	0xdd,
	InsCreatFile	= 	0xe0,
	InsDelFile		= 	0xe4,
	InsTermDF	= 	0xe6,
	InsTermEF	= 	0xe8,
	InsTermCard	= 	0xfe,
	EscUndef		=	0xff,
};

/* sw fields, standard
 * non volatile memory unchanged, by default it is unchanged unless 0x63 or 0x65
 */
enum{
	/* normal */
	Sw1Ok	=	0x90,
	Sw1More	=	0x61, /* sw2 encodes how many, should issue get response*/

	/* warning */
	Sw1RomUch	=	0x62,	/* non volatile memory unchanged */
	Sw1RomCh	=	0x63,	/* non volatile memory changed */
	Sw1SecIssue	=	0x66,
	
	/*Errors, no data*/

	/* exec error */
	Sw1ExRomUch	=	0x64,
	Sw1ExRomCh	=	0x65,
	Sw1ExSecIssue	=	0x66,

	/* chk error */
	Sw1iserr		=	0x67,	/* greater than this, error */

	Sw1Badlen	=	0x67,
	Sw1ClaFunUns	=	0x68,
	Sw1PermDen	=	0x69,
	Sw1BadParam	=	0x6a,
	Sw1BadParam1	=	0x6b,
	Sw1BadLe		=	0x6c,	/* sw2 says how many the same
							 * command with sw2 as short le must be issued
							*/
	Sw1BadInstr	=	0x6d,
	Sw1ClaUns	=	0x6e,
	Sw1UnknErr	=	0x6f,
	
};


enum{
	/* CLA constants */

	LastChCmd = 1 << 4,

	/* any of them 1 implies SM */
	SM = (1 << 5) | (1 << 3) | (1 << 2),

	Mask_B = 1 << 7, /* if 1, it is B*/

	HeadAuthSM_A = 1<< 2,

	HeadNAuthSM_A = (1 << 3),
	HeadNAuthSM_B = (1 << 5),

	PropSM_A	= 1 << 2,
	PropSM_A0	= 1 << 3,				/* have to  be 0*/
	
	LogicChan_A	= 0x03,				/* channels from 0 to 3 */
	LogicChan_B	= 0x0f,				/* channels from 4 to 19 */
};

enum{
	/* special file identifiers (like qids) remember id's have to be unique within DF (directory) */
	Rsvid0 = 0x0000,
	MFid = 0x3f00,						/* root of the hierarchy */
	Rsvid = 0x3ffff,
	Rfuid = 0xffff,
	
	/* short identifiers for EF, 5 bits (valid per EF or global) */
	Currshid = 0x00,
	Rsvshid = 0x1e,

	
};




CmdiccR *		iccrpc(int fd, Cmdicc *cm, int sz);
int		packcmdicc(uchar *b, int bsz, Cmdicc *c);
int		unpackcmdicc(uchar *b, int bsz, Cmdicc *c);
void		dumpicc(uchar *b, int sz, int send);
int		unpackcmdiccR(CmdiccR *c, uchar *b, int bsz);
int		procedraw(int fd, uchar *b, int sz);
CmdiccR *getresponse(int fd, int sz, uchar cla);
uchar	chan2cla(uchar chan);
