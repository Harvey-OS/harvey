typedef struct UnionDesc UnionDesc;
typedef struct LineCoding LineCoding;

/* Vendor and device IDs */
enum{
	/* Nokia mobile phones */
	NOKIAVid		= 0x0421,
	NOKIADidN95		= 0x0070,
};

/* USB interface classes */
enum{
	ComClass		= 0x02,
	DataClass		= 0x0a,
};

/* USB CDC descriptor types */
enum{
	HeaderType		= 0x00,
	ManagementType		= 0x01,
	ACMType			= 0x02,
	UnionType		= 0x06,
	CountryType		= 0x07,
	NetTerminalType		= 0x0a,
	EthernetType		= 0x0f,
	WHCMType		= 0x11,
	MDLMType		= 0x12,
	MDLMDetailType		= 0x13,
	DMMType			= 0x14,
	OBEXType		= 0x15,
	NCMType			= 0x1a,
};

/* "Union Functional Descriptor" from CDC spec 5.2.3.8 */
struct UnionDesc{
	uchar	bLength;
	uchar	bDescriptorType;
	uchar	bDescriptorSubType;
	uchar	bMasterInterface0;
	uchar	bSlaveInterface0;
	/* ... and there could be other slave interfaces */
};

/* Output control lines */
enum{
	CtrlDTR			= 0x01,
	CtrlRTS			= 0x02,
};

/* Capabilities from 5.2.3.3 */
enum{
	CommFeature		= 0x01,
	CapLine			= 0x02,
	CapBreak		= 0x04,
	CapNotify		= 0x08,
};

/*
 * Class-Specific Control Requests (6.2)
 * section 3.6.2.1 table 4 has the ACM profile, for modems.
 * */
enum{
	SendEncapsulatedCom	= 0x00,
	GetEncapsulatedResp	= 0x01,
	ReqSetLineCoding	= 0x20,
	ReqGetLineCoding	= 0x21,
	ReqSetCtrlLineState	= 0x22,
	ReqSendBreak		= 0x23,
};

/* Line Coding Structure from CDC spec 6.2.13 */
enum{
	StopBits1		= 0,
	StopBits15		= 1,
	StopBits2		= 2,

	NoParity		= 0,
	OddParity		= 1,
	EvenParity		= 2,
	MarkParity		= 3,
	SpaceParity		= 4,
};

struct LineCoding {
	uchar	dwDTERate[4];
	uchar	bCharFormat;
	uchar	bParityType;
	uchar	bDataBits;
};

extern Serialops acmops;
int 	acmmatch(char *);
int 	acmfindendpoints(Serial *);
