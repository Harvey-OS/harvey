typedef struct Mpsse Mpsse;

enum {
	KHz = 1000,
	FtdiHSpeedClk = 30*KHz,		/* 2232H 4232H */
	FtdiMaxClk = 60*KHz,		/* 2232C */
	FtdiReqTck = -1,
};

enum{
	isPushPull,
	notPushPull,
	isOpenDrain,
	notOpenDrain,
};

enum{
	MpsseBufSz	= 131072,	/* from openocd, noone knows why */
	MaxNbitsT	= 7,			/* maximum bits per state transition */
	MaxNbitsS	= 7,			/* maximum bits of data clocked */

	nOE = 0,
	TDI,
	TDO,
	TMS,
	TCK,

	TRSTnOE = 0,
	TRST,
	SRSTnOE,
	SRST,

	Sheeva = 0,
	GuruDisp = 1,
};


struct Mpsse{
	int nread;	/* bytes left to read */
	int nbits;	/* length of bits left to process */
	uchar *rb;	/* current point of buf processing */
	int rbits;	/* offset in the last bit to process counting from LSB */
	int lastbit;
	int motherb;
	int jtagfd;
	Biobufhdr bout;
	uchar bp[Bungetsize+MpsseBufSz];
};

int	mpsseflush(void *mdata);	/* internal, for ma.c */

/* from ma.c */
extern int	pushcmd(Mpsse *mpsse, char *ln);
extern int	pushcmdwdata(Mpsse *mpsse, char *ln, uchar *buf, int buflen);

/* from mpsse.c */
extern JMedium *	initmpsse(int fd, int motherb);
extern JMedium *	newmpsse(int fd, int motherb);
extern JMedium *	resetmpsse(JMedium *jmed);
