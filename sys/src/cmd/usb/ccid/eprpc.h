
/* msg types */
enum {
	/* interrupt */
	HardwareErrorTypI	= 0x51,
	NotifySlotChangeTypI= 0x50,

	/* requests */
	IccPowerOnTyp	= 0x62,
	IccPowerOffTyp	= 0x63,
	GetSlotStatusTyp	= 0x65,
	XfrBlockTyp		= 0x6F,
	GetParamTyp		= 0x6C,
	ResetParamTyp		= 0x6D,
	SetParamTyp		= 0x61,
	EscapeTyp		= 0x6B,
	IccClockTyp		= 0x6E,
	T0ApduTyp		= 0x6A,
	SecureTyp		= 0x69,
	MechanTyp		= 0x71,
	AbortTyp			= 0x72,
	SetDataRateClkTyp	= 0x73,

	/* responses */
	DataBlockTypR		= 0x80,
	SlotStatusTypR		= 0x81,
	ParamTypR		= 0x82,
	EscapeTypR		= 0x83,
	DataRateClkTypR	= 0x84,


	/* values of fields of the msgs */
	/* powsel */
	Voltauto = 0x00,
	Volt5,
	Volt3,
	Volt1_8,
	VoltMax,
	/* levelparam */
	Leveluniq = 0x0000,
	Levelstart,
	Levelend,
	Levelmid,
	Levelgimme = 0x0010,
	/* protoid */
	T0 = 0x00,
	T1,
	Twowire = 0x80,
	Threewire,
	I2C,
	/* clock cmd */
	Clockrestart = 0x00,
	Clockstop = 0x01,
	/* pinoperation */
	Pinverop = 0x00,
	Pinmodop,
	Pinbuftransop,
	PinwaitIccop,
	Pincancelop,
	Resendlastop,	/* T1 */
	Sendnextop,		/* T1 */
	/* SlotStatusTypR */
	NoIccpresent = 0x02,
	

	/*error codes */
	CmdNotSupp		= 0x00,
	Indexstart			= 0x01,
	Indexend			= 0x7F,
	CmdSlotBusy		= 0xE0,
	PinCancelled		= 0xEF,
	PinTimeout		= 0xF0,
	AutoseqBusy		= 0xF2,
	DeactProto		= 0xF3,
	ProcByteConflic	= 0xF4,
	IccClassNotSup	= 0xF5,
	IccProtoNotSup	= 0xF6,
	BadAtrTck		= 0xF7,
	BadAtrTs			= 0xF8,
	HwError			= 0xFB,
	XfrOverrun		= 0xFC,
	XfrParError		= 0xFD,
	IccMute			= 0xFE,
	Aborted			= 0xFF,

	/* sizes */
	Hdrsz = 10,
	IHdrsz = 1,
	Maxpayload = 0x48,
	/* greater than 8, so there is at least a byte in Ccid*/
	
};

typedef struct Cmsg Cmsg;
typedef struct Hdr Hdr;

struct Hdr{
	uchar	type;
	ulong	len;
	uchar bslot;
	uchar bseq;
	union {
		struct {
			uchar powsel;	/* IccPowerOn */
			uchar xx[2];
		};
		struct {
			uchar bwi;	/* XfrBlock Secure */
			ushort levelparam;	/* XfrBlock Secure */
			uchar yy;
		};
		struct {
			uchar protoid;	/* SetParam Param */
			uchar xx[2];
		};
		struct {
			uchar clockcmd;	/* IccClock */
			uchar xx[2];
		};
		struct {
			uchar mchanges;	/* T0Apdu */
			uchar classgetresp;	/* T0Apdu */
			uchar classenv;		/* T0Apdu */
		};
		struct {
			uchar function;		/* Mechan */
			uchar xx[2];
		};
		struct {
			uchar status;			/* Responses */ 
			uchar error;
			char *errstr;	
			union{
				uchar chainparam;	/* DataBlock R */ 
				uchar clockstatus;	/* SlotStatus  R*/
			};
		};
	};
	
};

struct Cmsg{
	int isresp;
	int isabort;
	Hdr;
	void	*data;
	uchar	unpkdata[Maxpayload];
};

typedef struct Imsg Imsg;
struct Imsg{
	uchar	type;
	union {
		struct {		
			uchar bslot;		/* HardwareError */
			uchar bseq;		/* HardwareError */
			uchar hwerrcode;	/* HardwareError */
		};
		struct {
			ulong sloticcstate;	/* 2 bits per slot NotifySlotChange */
		};
	};
};


/* stuff that goes in Cmsg.data (ICC protocols notwithstanding) */

/* SetParameters: declared in ccid.h so that it can be used to keep
 * the parameters
 */

/* Secure */

typedef struct Pinop Pinop;
typedef struct Pinmod Pinmod;
typedef struct Pinver Pinver;

/* Careful, unordered */

struct Pinver{
	uchar msgidx;
};
struct Pinmod{
	uchar insertoffold;
	uchar insertoffnew;
	uchar confirmpin;
	uchar msgidx1;
	uchar msgidx2;
	uchar msgidx3;
};


struct Pinop{
	uchar pinoperation;
	uchar tout;
	uchar fmtstr;
	uchar pinblkstr;
	uchar pinlenfmt;
	ushort pinxdigitmax;
	uchar entryvalcond;
	uchar nmsg;
	uchar langid;
	union{
		Pinmod;
		Pinver;
	};
	uchar teoprolog[3];
	uchar *apinapdu;
};

/* DataRateClkTyp */
typedef struct Clockdata Clockdata;
struct Clockdata{
	ulong clockfreq;
	ulong datarate;
};

int iccgetslstatus(int fdout, int fdin);
int iccpoweron(int fdout, int fdin);
//int iccxferblock(int fdout, int fdin);
Cmsg *iccmsgrpc(Ccid *ccid, void *blk, int sz, uchar type, uchar twait);
int unpackimsg(Imsg *im, uchar *b, int bsz);
int	isiccpresent(Ccid *ccid);