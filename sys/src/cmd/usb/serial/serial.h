typedef struct Serialops Serialops;
typedef struct Serial Serial;

struct Serialops {
	int	(*init)(Serial*);
	int	(*getparam)(Serial*);
	int	(*setparam)(Serial*);
	int	(*clearpipes)(Serial*);
	int	(*sendlines)(Serial*);
	int	(*modemctl)(Serial*, int);
	int	(*setbreak)(Serial*, int);
	int	(*readstatus)(Serial*);
};

struct Serial {
	QLock;
	Dev	*dev;		/* usb device*/
	Dev	*ep;		/* endpoint to get events */
	Dev	*epintr;
	Dev	*epin;
	Dev	*epout;
	Usbfs	fs;
	int	type;
	int	recover;
	int	hasepintr;

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
	vlong	timer;
	int	blocked;	/* for sw flow ctl. BUG: not implemented yet */
	int	nbreakerr;
	int	ring;
	int	nframeerr;
	int	nparityerr;
	int	novererr;
	int	enabled;

	Serialops;
};

enum {
	/* soft flow control chars */
	CTLS = 023,
	CTLQ = 021,
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

int	serialrecover(Serial *ser, char *err);
int	serialreset(Serial *ser);
char	*serdumpst(Serial *ser, char *buf, int bufsz);

int	serialnop(Serial *);
int	serialnopctl(Serial *, int);
