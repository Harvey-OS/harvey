typedef struct Serial Serial;
typedef struct Serialops Serialops;
typedef struct Serialport Serialport;

struct Serialops {
	int	(*seteps)(Serialport*);
	int	(*init)(Serialport*);
	int	(*getparam)(Serialport*);
	int	(*setparam)(Serialport*);
	int	(*clearpipes)(Serialport*);
	int	(*reset)(Serial*, Serialport*);
	int	(*sendlines)(Serialport*);
	int	(*modemctl)(Serialport*, int);
	int	(*setbreak)(Serialport*, int);
	int	(*readstatus)(Serialport*);
	int	(*wait4data)(Serialport*, uchar *, int);
	int	(*wait4write)(Serialport*, uchar *, int);
};

enum {
	DataBufSz = 8*1024,
	Maxifc = 16,
};

struct Serialport {
	char name[32];
	Serial	*s;		/* device we belong to */
	int	isjtag;

	Dev	*epintr;	/* may not exist */

	Dev	*epin;
	Dev	*epout;

	uchar	ctlstate;

	/* serial parameters */
	uint	baud;
	int	stop;
	int	mctl;
	int	parity;
	int	bits;
	int	fifo;
	int	limit;
	int	rts;
	int	cts;
	int	dsr;
	int	dcd;
	int	dtr;
	int	rlsd;

	vlong	timer;
	int	blocked;	/* for sw flow ctl. BUG: not implemented yet */
	int	nbreakerr;
	int	ring;
	int	nframeerr;
	int	nparityerr;
	int	novererr;
	int	enabled;

	int	interfc;	/* interfc on the device for ftdi */

	Channel *w4data;
	Channel *gotdata;
	int	ndata;
	uchar	data[DataBufSz];

	Reqqueue *rq;
	Reqqueue *wq;
};

struct Serial {
	QLock;
	Dev	*dev;		/* usb device*/

	int	type;		/* serial model subtype */
	int	recover;	/* # of non-fatal recovery tries */
	Serialops;

	int	hasepintr;

	int	jtag;		/* index of jtag interface, -1 none */
	int	nifcs;		/* # of serial interfaces, including JTAG */
	Serialport p[Maxifc];
	int	maxrtrans;
	int	maxwtrans;

	int	maxread;
	int	maxwrite;

	int	inhdrsz;
	int	outhdrsz;
	int	baudbase;	/* for special baud base settings, see ftdi */
};

enum {
	/* soft flow control chars */
	CTLS	= 023,
	CTLQ	= 021,
	CtlDTR	= 1,
	CtlRTS	= 2,
};

/*
 * !hget http://lxr.linux.no/source/drivers/usb/serial/pl2303.h|htmlfmt
 * !hget http://lxr.linux.no/source/drivers/usb/serial/pl2303.c|htmlfmt
 */

typedef struct Cinfo Cinfo;
struct Cinfo {
	int	vid;		/* usb vendor id */
	int	did;		/* usb device/product id */

	int	cid;		/* assigned for us */
};

extern int serialdebug;

#define	dsprint	if(serialdebug)fprint

int	serialrecover(Serial *ser, Serialport *p, Dev *ep, char *err);
int	serialreset(Serial *ser);
char	*serdumpst(Serialport *p, char *buf, int bufsz);
Cinfo	*matchid(Cinfo *tab, int vid, int did);
