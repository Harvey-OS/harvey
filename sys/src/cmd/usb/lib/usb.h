/*
 * USB implementation for Plan 9
 *	(c) 1998, 1999 C H Forsyth
 */

enum {
	Dbginfo = 0x01,
	Dbgfs = 0x02,
	Dbgproc = 0x04,
	Dbgcontrol = 0x08,
};

extern int debug, debugdebug, verbose;

typedef uchar byte;

#ifndef CHANNOP
typedef struct Ref Ref;

#define threadprint fprint
#endif

/*
 * USB definitions
 */

typedef struct DConfig DConfig;
typedef struct DDevice DDevice;
typedef struct DEndpoint DEndpoint;
typedef struct DHid DHid;
typedef struct DHub DHub;
typedef struct DInterface DInterface;
typedef struct Dconf Dconf;
typedef struct Dalt Dalt;
typedef struct Device Device;
typedef struct Dinf Dinf;
typedef struct Endpt Endpt;

typedef struct Namelist Namelist;

#ifndef nelem
#define	nelem(x)	(sizeof((x))/sizeof((x)[0]))
#endif

#define	GET2(p)	((((p)[1]&0xFF)<<8)|((p)[0]&0xFF))
#define	PUT2(p,v)	{((p)[0] = (v)); ((p)[1] = (v)>>8);}

enum
{
	TokIN = 0x69,
	TokOUT = 0xE1,
	TokSETUP = 0x2D,

	/* request type */
	RH2D = 0<<7,
	RD2H = 1<<7,
	Rstandard = 0<<5,
	Rclass = 1<<5,
	Rvendor = 2<<5,
	Rdevice = 0,
	Rinterface = 1,
	Rendpt = 2,
	Rother = 3,

	/* standard requests */
	GET_STATUS = 0,
	CLEAR_FEATURE = 1,
	SET_FEATURE = 3,
	SET_ADDRESS = 5,
	GET_DESCRIPTOR = 6,
	SET_DESCRIPTOR = 7,
	GET_CONFIGURATION = 8,
	SET_CONFIGURATION = 9,
	GET_INTERFACE = 10,
	SET_INTERFACE = 11,
	SYNCH_FRAME = 12,

	GET_CUR = 0x81,
	GET_MIN = 0x82,
	GET_MAX = 0x83,
	GET_RES = 0x84,
	SET_CUR = 0x01,
	SET_MIN = 0x02,
	SET_MAX = 0x03,
	SET_RES = 0x04,

	/* hub class feature selectors */
	C_HUB_LOCAL_POWER = 0,
	C_HUB_OVER_CURRENT,
	PORT_CONNECTION = 0,
	PORT_ENABLE = 1,
	PORT_SUSPEND = 2,
	PORT_OVER_CURRENT = 3,
	PORT_RESET = 4,
	PORT_POWER = 8,
	PORT_LOW_SPEED = 9,
	C_PORT_CONNECTION = 16,
	C_PORT_ENABLE,
	C_PORT_SUSPEND,
	C_PORT_OVER_CURRENT,
	C_PORT_RESET,

	/* descriptor types */
	DEVICE = 1,
	CONFIGURATION = 2,
	STRING = 3,
	INTERFACE = 4,
	ENDPOINT = 5,
	HID = 0x21,
	REPORT = 0x22,
	PHYSICAL = 0x23,
	HUB	= 0x29,

	/* feature selectors */
	DEVICE_REMOTE_WAKEUP = 1,
	ENDPOINT_STALL = 0,

	/* report types */
	Tmtype = 3<<2,
	Tmitem = 0xF0,
	Tmain = 0<<2,
		Tinput = 0x80,
		Toutput = 0x90,
		Tfeature = 0xB0,
		Tcoll = 0xA0,
		Tecoll = 0xC0,
	 Tglobal = 1<<2,
		Tusagepage = 0x00,
		Tlmin = 0x10,
		Tlmax = 0x20,
		Tpmin = 0x30,
		Tpmax = 0x40,
		Tunitexp = 0x50,
		Tunit = 0x60,
		Trepsize = 0x70,
		TrepID = 0x80,
		Trepcount = 0x90,
		Tpush = 0xA0,
		Tpop = 0xB0,
	 Tlocal = 2<<2,
		Tusage = 0x00,
		Tumin = 0x10,
		Tumax = 0x20,
		Tdindex = 0x30,
		Tdmin = 0x40,
		Tdmax = 0x50,
		Tsindex = 0x70,
		Tsmin = 0x80,
		Tsmax = 0x90,
		Tsetdelim = 0xA0,
	 Treserved = 3<<2,
	 Tlong = 0xFE,

	/* parameters */
	Nendpt =	16,

	/* device state */
	Detached = 0,
	Attached,
	Enabled,
	Assigned,
	Configured,

	/* classes */
	Noclass = 0,
	Hubclass,
	Otherclass,

	/* endpoint direction */
	Ein = 0,
	Eout,

	/* endpoint type */
	Econtrol = 0,
	Eiso = 1,
	Ebulk = 2,
	Eintr = 3,

	/* endpoint isotype */
	Eunknown = 0,
	Easync = 1,
	Eadapt = 2,
	Esync = 3,
};

enum
{
	CL_AUDIO = 1,
	CL_COMMS = 2,
	CL_HID = 3,
	CL_PRINTER = 7,
	CL_STORAGE = 8,
	CL_HUB = 9,
	CL_DATA = 10,
};

struct Endpt
{
	uchar	addr;		/* endpoint address, 0-15 (|0x80 if direction==Ein) */
	uchar	dir;		/* direction, Ein/Eout */
	uchar	type;		/* Econtrol, Eiso, Ebulk, Eintr */
	uchar	isotype;	/* Eunknown, Easync, Eadapt, Esync */
	int	id;
	int	class;
	ulong	csp;
	int	maxpkt;
	Device*	dev;
	Dconf*	conf;
	Dinf*	iface;
};

struct Dalt
{
	int	attrib;
	int	interval;
	void*	devspec;	/* device specific settings */
};

struct Dinf
{
	int 	interface;	/* interface number */
	ulong	csp;		/* USB class/subclass/proto */
	Dalt*	dalt[16];
	Endpt*	endpt[16];
};

struct Dconf
{
	ulong	csp;		/* USB class/subclass/proto */
	int	nif;		/* number of interfaces */
	int	cval;		/* value for set configuration */
	int	attrib;
	int	milliamps;	/* maximum power in this configuration */
	Dinf*	iface[16];	/* up to 16 interfaces */
};

/* Dconf.attrib */
enum
{
	Cbuspowered = 1<<7,
	Cselfpowered = 1<<6,
	Cremotewakeup = 1<<5,
};
	
struct Device
{
	Ref;
	int	ctlrno;
	int	ctl;
	int	setup;
	int	status;
	int	state;
	int	id;
	int	class;
	int	npt;
	int	ls;		/* low speed */
	ulong	csp;		/* USB class/subclass/proto */
	int	nconf;
	int	nif;		/* number of interfaces (sum of per-conf `nif's) */
	int	vid;		/* vendor id */
	int	did;		/* product (device) id */
	Dconf*	config[16];
	Endpt*	ep[Nendpt];
};

/*
 * layout of standard descriptor types
 */
struct DDevice
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bcdUSB[2];
	byte	bDeviceClass;
	byte	bDeviceSubClass;
	byte	bDeviceProtocol;
	byte	bMaxPacketSize0;
	byte	idVendor[2];
	byte	idProduct[2];
	byte	bcdDevice[2];
	byte	iManufacturer;
	byte	iProduct;
	byte	iSerialNumber;
	byte	bNumConfigurations;
};
#define	DDEVLEN	18

struct DConfig
{
	byte	bLength;
	byte	bDescriptorType;
	byte	wTotalLength[2];
	byte	bNumInterfaces;
	byte	bConfigurationValue;
	byte	iConfiguration;
	byte	bmAttributes;
	byte	MaxPower;
};
#define	DCONFLEN	9

struct DInterface
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bInterfaceNumber;
	byte	bAlternateSetting;
	byte	bNumEndpoints;
	byte	bInterfaceClass;
	byte	bInterfaceSubClass;
	byte	bInterfaceProtocol;
	byte	iInterface;
};
#define	DINTERLEN	9

struct DEndpoint
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bEndpointAddress;
	byte	bmAttributes;
	byte	wMaxPacketSize[2];
	byte	bInterval;
};
#define	DENDPLEN	7

struct DHid
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bcdHID[2];
	byte	bCountryCode;
	byte	bNumDescriptors;
	byte	bClassDescriptorType;
	byte	wItemLength[2];
};
#define	DHIDLEN	9

struct DHub
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bNbrPorts;
	byte	wHubCharacteristics[2];
	byte	bPwrOn2PwrGood;
	byte	bHubContrCurrent;
	byte	DeviceRemovable[1];	/* variable length */
/*	byte	PortPwrCtrlMask;		/* variable length, deprecated in USB v1.1 */
};
#define	DHUBLEN	9

struct Namelist
{
	short	index;
	char		*name;
};

typedef struct Drivetab
{
	ulong	csp;
	void	(*driver)(Device *d);
} Drivetab;

#define Class(csp)		((csp)&0xff)
#define Subclass(csp)	(((csp)>>8)&0xff)
#define Proto(csp)		(((csp)>>16)&0xff)
#define CSP(c, s, p)	((c) | ((s)<<8) | ((p)<<16))

extern void (*dprinter[0x100])(Device *, int, ulong, void *b, int n);

/*
 * format routines
 */
void	pdesc	(Device *, int, ulong, byte *, int);
void	preport	(Device *, int, ulong, byte *, int);
void	pstring	(Device *, int, ulong, void *, int);
void	phub	(Device *, int, ulong, void *, int);
void	pdevice	(Device *, int, ulong, void *, int);
void	phid	(Device *, int, ulong, void *, int);
void	pcs_raw(char *tag, byte *b, int n);

/*
 * interface
 */
void	usbfmtinit(void);
Device*	opendev(int, int);
void	closedev(Device*);
int	describedevice(Device*);
int	loadconfig(Device *d, int n);
Endpt *	newendpt(Device *d, int id, ulong csp);
int	setupcmd(Endpt*, int, int, int, int, byte*, int);
int	setupreq(Endpt*, int, int, int, int, int);
int	setupreply(Endpt*, void*, int);
void	setdevclass(Device *d, int n);
void *	emalloc(ulong);
void *	emallocz(ulong, int);

char *	namefor(Namelist *, int);

#pragma	varargck	type  "D"	Device*
