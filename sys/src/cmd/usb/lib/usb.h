typedef struct Device Device;
typedef struct Dconf Dconf;
typedef struct Ddesc Ddesc;
typedef struct Dinf Dinf;
typedef struct Dalt Dalt;
typedef struct Endpt Endpt;

struct Device
{
	Ref;
	int		ctlrno;
	int		id;
	int		state;
	int		ctl;
	int		setup;
	int		status;
	int		ls;			/* low speed */

	int		vers;			/* USB version supported, in BCD */
	ulong	csp;			/* USB class/subclass/proto */
	int		max0;		/* max packet size for endpoint 0 */
	int		vid;			/* vendor id */
	int		did;			/* product (device) id */
	int		release;		/* device release number, in BCD */
	int		manufacturer;	/* string index */
	int		product;		/* string index */
	int		serial;		/* string index */
	int		nconf;
	Dconf*	config;
	char		*strings[256];
};

struct Ddesc
{
	int		ndesc;		/* number of descriptors */
	int		bytes;		/* total size of descriptors */
	uchar	*data;		/* descriptor data */
};

struct Dconf
{
	Device	*d;			/* owning device */
	int		x;			/* index into Device.config array */
	int		nif;			/* number of interfaces */
	int		cval;			/* value for set configuration request */
	int		config;		/* string index */
	int		attrib;
	int		milliamps;	/* maximum power in this configuration */
	Ddesc	desc;		/* additional descriptors for this configuration */
	Dinf*	iface;
};

struct Dinf
{
	Device	*d;			/* owning device */
	Dconf	*conf;		/* owning configuration */
	int		x;			/* index into Dconf.iface array */
	ulong	csp;			/* USB class/subclass/proto */
	int		interface;		/* string index */
	Dalt		*alt;			/* linked list of alternatives */
	Dinf		*next;		/* caller-maintained list of interfaces */
};

struct Dalt
{
	Dinf		*intf;			/* owning interface */
	int		alt;			/* alternate number, used to select this alternate */
	int		npt;			/* number of endpoints */
	Endpt*	ep;			/* endpoints for this interface alternate */
	Ddesc	desc;		/* additional descriptors for this alternate */
	Dalt*		next;			/* next in linked list of alternatives */
};

struct Endpt
{
	uchar	addr;		/* endpoint address, 0-15 */
	uchar	dir;			/* direction, Ein/Eout */
	uchar	type;			/* Econtrol, Eiso, Ebulk, Eintr */
	uchar	isotype;		/* Eunknown, Easync, Eadapt, Esync */
	int		maxpkt;		/* maximum packet size for endpoint */
	int		pollms;		/* polling interval for interrupts */
	Ddesc	desc;		/* additional descriptors for this endpoint */
};

enum
{
	/* Device.state */
	Detached = 0,
	Attached,
	Enabled,
	Assigned,
	Configured,

	/* Dconf.attrib */
	Cbuspowered = 1<<7,
	Cselfpowered = 1<<6,
	Cremotewakeup = 1<<5,

	/* Endpt.dir */
	Ein = 0,
	Eout,

	/* Endpt.type */
	Econtrol = 0,
	Eiso,
	Ebulk,
	Eintr,

	/* Endpt.isotype */
	Eunknown = 0,
	Easync,
	Eadapt,
	Esync,
};

#define Class(csp)		((csp)&0xff)
#define Subclass(csp)	(((csp)>>8)&0xff)
#define Proto(csp)		(((csp)>>16)&0xff)
#define CSP(c, s, p)	((c) | ((s)<<8) | ((p)<<16))

enum
{
	/* known classes */
	CL_AUDIO = 1,
	CL_COMMS = 2,
	CL_HID = 3,
	CL_PRINTER = 7,
	CL_HUB = 9,
	CL_DATA = 10,
};

/*
 * interface
 */
void		usbfmtinit(void);
Device*	opendev(int, int);
void		closedev(Device*);
int		describedevice(Device*);
void		dumpdevice(int, Device*);
void		loadstr(Device*, int, int);
void		loadstrings(Device*, int);

int		setupreq(Device*, int, int, int, int, uchar*, int);

void	*	emalloc(ulong);
void	*	emallocz(ulong, int);

#pragma	varargck	type  "D"	Device*
