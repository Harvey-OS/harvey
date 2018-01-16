/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	int	(*wait4data)(Serialport*, uint8_t *, int);
	int	(*wait4write)(Serialport*, uint8_t *, int);
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

	Usbfs	fs;
	uint8_t	ctlstate;

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

	int64_t	timer;
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
	Channel *readc;		/* to uncouple reads, only used in ftdi... */
	int	ndata;
	uint8_t	data[DataBufSz];
};

struct Serial {
	QLock	QLock;
	Dev	*dev;		/* usb device*/

	int	type;		/* serial model subtype */
	int	recover;	/* # of non-fatal recovery tries */
	Serialops	Serialops;

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

int serialmain(Dev *d, int argc, char *argv[]);

typedef struct Cinfo Cinfo;
struct Cinfo {
	int	vid;		/* usb vendor id */
	int	did;		/* usb device/product id */
	int	cid;		/* controller id assigned by us */
};

extern Cinfo plinfo[];
extern Cinfo uconsinfo[];
extern int serialdebug;

#define	dsprint	if(serialdebug)fprint

int	serialrecover(Serial *ser, Serialport *p, Dev *ep, char *err);
int	serialreset(Serial *ser);
char	*serdumpst(Serialport *p, char *buf, int bufsz);
