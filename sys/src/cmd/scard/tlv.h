/* we don't do compact yet though they are easy
 *  a tag of 4X and a len of 0Y are merged into an XY
*/

enum{
	MaxTlvData = 128,

	TSimple = 0,
	TComp,
	TBer
};
typedef struct Tlv Tlv;
typedef struct Tagstr Tagstr;

struct Tagstr{
	uchar tag;
	char	str[128];
};

struct Tlv{
	int	kind;
	uchar	tagclass;
	int	isconstr;

	ulong	tag;
	ulong	len;
	uchar	*data;
	
	uchar datapack[MaxTlvData];
};

enum{
	TagMsk = 0x1f,
	IsConstruct = 1 << 5,
	AppClass = 1,
	CtxClass = 2,
	PrivClass = 3,
	TagIs3B = 1 << 7,

	MaxBer1B = 0x7f,
	Ber2B = 0x81,
	Ber3B,
	Ber4B,
	Ber5B,

	TagObj = 0x06,
	TagCCode = 0x41,
	TagIssuer = 0x42,
	TagAid = 0x4f,
	TagFref = 0x51,

	TagFcp = 0x62,
	TagFmd = 0x64,
	TagFci = 0x6f,

	TagAllocID = 0x78,

	/* File Control parameter data object tags, see fcpstr for strings */
	TagFlen = 0x80,
	TagFMlen = 0x81,
	TagFd = 0x82,
	TagFid = 0x83,
	TagDFname = 0x84,
	TagProp = 0x85,
	TagSecProp = 0x86,
	TagSecEFExtra = 0x87,
	TagShort = 0x88,
	TagLcs = 0x8A,
	TagSecAttrRef = 0x8b,
	TagSecAttrComp = 0x8c,
	TagEFSecEnv = 0x8d,
	TacSecChan = 0x8e,
	TagSecAttrData = 0xa0,
	TagSecProp2 = 0xa1,
	TagEFlist = 0xa2,
	TagPropTlv = 0xa5,
	TagSectAttrExp = 0xab,
	TagCryptId = 0xac,
	
	TagInval = 0xff,

	/* Expanded security attr tags */
	TagAlways = 0x90,
	TagNever = 0x97,
	TagScSe = 0x9e,
	TagAuthTemp = 0xa4,
	TagAllowifOr = 0xa0,
	TagAllowifNot = 0xa7,
	TagAllowifAnd = 0xaf,

	TagIdKey = 0x83,
	TagAuthVer = 0x95,
};
int		packtlv(Tlv *t, uchar *b, int bsz);
int		unpacktlv(Tlv *t, uchar *b, int bsz, int kind);
void		tagpr2levels(uchar *buf, int bufsz, Tagstr *ts);

extern Tagstr fcpstr[];