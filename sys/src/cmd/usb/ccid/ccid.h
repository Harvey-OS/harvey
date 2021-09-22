typedef struct Iccops Iccops;
typedef struct Ccid Ccid;
typedef struct Cciddesc Cciddesc;
typedef struct Slot Slot;
typedef struct Client Client;
typedef struct ParsedAtr ParsedAtr;
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
};

struct Iccops {
	int	(*init)(Ccid*);
};

extern Iccops defops;

/* descriptor values */
enum{
	DVolt5 = 0x01,
	DVolt3 = 0x02,
	DVolt1_8 = 0x04,
	DVoltMax = 0x08,
	
	Dmaxstr = 512, /* max printing size of desc wc + extras :-) */

	Maxcli = 32,
	Stacksz = 32*1024,
};

struct Cciddesc {
	ushort	release;
	uchar	maxslot;
	uchar	voltsup;
	ulong	protomsk;
	ulong	defclock;
	ulong	maxclock;
	uchar	nclocks;
	ulong	datarate;
	ulong	maxdatarate;
	uchar	ndatarates;
	ulong	maxifsd;
	ulong	synchproto;
	ulong	mechanical;
	ulong	features;

	ulong	maxmsglen;
	uchar	classgetresp;
	uchar	classenvel;
	ushort	lcdlayout;
	uchar	pinsupport;
	uchar	maxbusyslots;
};

struct Slot{
	char *desc; /* string in human readable from scard.txt */
	uchar *atr;
	int natr;
	ParsedAtr patr;
	Param	param;
	Iccops;
	int nreq;
};

struct Client{
	int ncli;	/* index in ccid for reverse lookup */
	Ccid *ccid;
	int halfrpc;
	Channel *to;
	Channel *from;
	Channel *start;
	uchar *atr;
	int natr;
	Slot *sl;
};


struct Ccid {
	QLock;
	Dev	*dev;		/* usb device*/
	Dev	*ep;		/* endpoint to get events */
	int	hasintr;	/* intr endpoint is optional */
	Dev	*epintr;
	Dev	*epin;
	Dev	*epout;
	Usbfs	fs;
	int	type;

	int israwopen;
	int isrpcopen;
	int recover;
	Cciddesc;
		/* seq counter BUG: is this per ccid or per slot? */
	Slot *sl;	/* nil if slot 0 not active, only one supported at the moment...*/

	Client *cl[Maxcli];
};

enum {
	CcidCSP		= 0x0B0000,	/* smartcard.nil.CCID */
	CcidCSP2		= 0x00000B,	/* smartcard.nil.CCID */

	CcidDesclen	= 0x36,
	CcidDesctype	= 0x21,
	
	/* class specific resquests */
	CcidAbortcmd	= 0x1,	
	CcidGetclock	= 0x2,	/* unimpl */
	CcidGetrates	= 0x3,	/* unimpl */
};

int ccidmain(Dev *d, int argc, char *argv[]);

typedef struct Cinfo Cinfo;
struct Cinfo {
	int	vid;		/* usb vendor id */
	int	did;		/* usb device/product id */
	int	cid;		/* controller id assigned by us */
};

extern Cinfo cinfo[];
extern int cciddebug;

#define	dcprint	if(cciddebug == 0){} else fprint

char *lookupatr(uchar *data, int sz);
int	ccidrecover(Ccid *ccid, char *err);
int	ccidabort(Ccid *ser, uchar bseq, uchar bslot, uchar *buf);
int		ccidslotinit(Ccid *ccid, int nsl);
void		ccidslotfree(Ccid *ccid, int nsl);

void	clientproc(void *u);
Client *fillclient(Client *, uchar *atr, int atrsz);
int	matchatr(uchar *atr, uchar *atr2, int sz);
Client *matchclient(Ccid *ccid, uchar *atr, int atrsz);
int	matchcard(Ccid *ccid, uchar *atr, int atrsz);
void	statusreader(void *u);