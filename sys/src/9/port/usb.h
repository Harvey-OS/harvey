/*
 * common USB definitions.
 */
#define dprint		if(debug)print
#define ddprint		if(debug>1)print
#define deprint		if(debug || ep->debug)print
#define ddeprint	if(debug>1 || ep->debug>1)print

#define	GET2(p)		((((p)[1]&0xFF)<<8)|((p)[0]&0xFF))
#define	PUT2(p,v)	{((p)[0] = (v)); ((p)[1] = (v)>>8);}

typedef struct Udev Udev;	/* USB device */
typedef struct Ep Ep;		/* Endpoint */
typedef struct Hci Hci;		/* Host Controller Interface */
typedef struct Hciimpl Hciimpl;	/* Link to the controller impl. */

enum
{
	/* fundamental constants */
	Ndeveps	= 16,		/* max nb. of endpoints per device */

	/* tunable parameters */
	Nhcis	= 16,		/* max nb. of HCIs */
	Neps	= 128,		/* max nb. of endpoints */
	Maxctllen = 32*1024, /* max allowed sized for ctl. xfers; see Maxdevconf */
	Xfertmout = 2000,	/* default request time out (ms) */

	/* transfer types. keep this order */
	Tnone = 0,		/* no tranfer type configured */
	Tctl,			/* wr req + rd/wr data + wr/rd sts */
	Tiso,			/* stream rd or wr (real time) */
	Tbulk,			/* stream rd or wr */
	Tintr,			/* msg rd or wr */
	Nttypes,		/* number of transfer types */

	Epmax	= 0xF,		/* max ep. addr */
	Devmax	= 0x7F,		/* max dev. addr */

	/* Speeds */
	Fullspeed = 0,
	Lowspeed,
	Highspeed,
	Nospeed,

	/* request type */
	Rh2d = 0<<7,
	Rd2h = 1<<7,
	Rstd = 0<<5,
	Rclass =  1<<5,
	Rdev = 0,
	Rep = 2,
	Rother = 3,

	/* req offsets */
	Rtype	= 0,
	Rreq	= 1,
	Rvalue	= 2,
	Rindex	= 4,
	Rcount	= 6,
	Rsetuplen = 8,

	/* standard requests */
	Rgetstatus	= 0,
	Rclearfeature	= 1,
	Rsetfeature	= 3,
	Rsetaddr	= 5,
	Rgetdesc	= 6,

	/* device states */
	Dconfig	 = 0,		/* configuration in progress */
	Denabled,		/* address assigned */
	Ddetach,		/* device is detached */
	Dreset,			/* its port is being reset */

	/* (root) Hub reply to port status (reported to usbd) */
	HPpresent	= 0x1,
	HPenable	= 0x2,
	HPsuspend	= 0x4,
	HPovercurrent	= 0x8,
	HPreset		= 0x10,
	HPpower		= 0x100,
	HPslow		= 0x200,
	HPhigh		= 0x400,
	HPstatuschg	= 0x10000,
	HPchange	= 0x20000,
};

/*
 * Services provided by the driver.
 * epopen allocates hardware structures to prepare the endpoint
 * for I/O. This happens when the user opens the data file.
 * epclose releases them. This happens when the data file is closed.
 * epwrite tries to write the given bytes, waiting until all of them
 * have been written (or failed) before returning; but not for Iso.
 * epread does the same for reading.
 * It can be assumed that endpoints are DMEXCL but concurrent
 * read/writes may be issued and the controller must take care.
 * For control endpoints, device-to-host requests must be followed by
 * a read of the expected length if needed.
 * The port requests are called when usbd issues commands for root
 * hubs. Port status must return bits as a hub request would do.
 * Toggle handling and other details are left for the controller driver
 * to avoid mixing too much the controller and the comon device.
 * While an endpoint is closed, its toggles are saved in the Ep struct.
 */
struct Hciimpl
{
	void	*aux;				/* for controller info */
	void	(*init)(Hci*);			/* init. controller */
	void	(*dump)(Hci*);			/* debug */
	void	(*interrupt)(Ureg*, void*);	/* service interrupt */
	void	(*epopen)(Ep*);			/* prepare ep. for I/O */
	void	(*epclose)(Ep*);		/* terminate I/O on ep. */
	long	(*epread)(Ep*,void*,long);	/* transmit data for ep */
	long	(*epwrite)(Ep*,void*,long);	/* receive data for ep */
	char*	(*seprintep)(char*,char*,Ep*);	/* debug */
	int	(*portenable)(Hci*, int, int);	/* enable/disable port */
	int	(*portreset)(Hci*, int, int);	/* set/clear port reset */
	int	(*portstatus)(Hci*, int);	/* get port status */
	void	(*shutdown)(Hci*);		/* shutdown for reboot */
	void	(*debug)(Hci*, int);		/* set/clear debug flag */
};

struct Hci
{
	ISAConf;				/* hardware info */
	int	tbdf;				/* type+busno+devno+funcno */
	int	ctlrno;				/* controller number */
	int	nports;				/* number of ports in hub */
	int	highspeed;
	Hciimpl;					/* HCI driver  */
};

/*
 * USB endpoint.
 * All endpoints are kept in a global array. The first
 * block of fields is constant after endpoint creation.
 * The rest is configuration information given to all controllers.
 * The first endpoint for a device (known as ep0) represents the
 * device and is used to configure it and create other endpoints.
 * Its QLock also protects per-device data in dev.
 * See Hciimpl for clues regarding how this is used by controllers.
 */
struct Ep
{
	Ref;			/* one per fid (and per dev ep for ep0s) */

	/* const once inited. */
	int	idx;		/* index in global eps array */
	int	nb;		/* endpoint number in device */
	Hci*	hp;		/* HCI it belongs to */
	Udev*	dev;		/* device for the endpoint */
	Ep*	ep0;		/* control endpoint for its device */

	QLock;			/* protect fields below */
	char*	name;		/* for ep file names at #u/ */
	int	inuse;		/* endpoint is open */
	int	mode;		/* OREAD, OWRITE, or ORDWR */
	int	clrhalt;	/* true if halt was cleared on ep. */
	int	debug;		/* per endpoint debug flag */
	char*	info;		/* for humans to read */
	long	maxpkt;		/* maximum packet size */
	int	ttype;		/* tranfer type */
	ulong	load;		/* in µs, for a fransfer of maxpkt bytes */
	void*	aux;		/* for controller specific info */
	int	rhrepl;		/* fake root hub replies */
	int	toggle[2];	/* saved toggles (while ep is not in use) */
	long	pollival;		/* poll interval ([µ]frames; intr/iso) */
	long	hz;		/* poll frequency (iso) */
	long	samplesz;	/* sample size (iso) */
	int	ntds;		/* nb. of Tds per µframe */
	int	tmout;		/* 0 or timeout for transfers (ms) */
};

/*
 * Per-device configuration and cached list of endpoints.
 * eps[0]->QLock protects it.
 */
struct Udev
{
	int	nb;		/* USB device number */
	int	state;		/* state for the device */
	int	ishub;		/* hubs can allocate devices */
	int	isroot;		/* is a root hub */
	int	speed;		/* Full/Low/High/No -speed */
	int	hub;		/* dev number for the parent hub */
	int	port;		/* port number in the parent hub */
	Ep*	eps[Ndeveps];	/* end points for this device (cached) */
};

void	addhcitype(char *type, int (*reset)(Hci*));

extern char *usbmodename[];
extern char Estalled[];

extern char *seprintdata(char*,char*,uchar*,int);
