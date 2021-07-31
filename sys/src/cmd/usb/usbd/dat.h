typedef struct Hub Hub;
typedef struct Port Port;

struct Hub
{
	byte		pwrmode;
	byte		compound;
	byte		pwrms;		/* time to wait in ms after powering port */
	byte		maxcurrent;
	byte		nport;
	Port		*port;

	int		isroot;		/* set if this hub is a root hub */
	int		portfd;		/* fd of /dev/usb%d/port if root hub */
	int		ctlrno;		/* number of controller this hub is on */
	Device*	dev0;		/* device 0 of controller */
	Device*	d;			/* device of hub (same as dev0 for root hub) */
};

struct Port
{
	byte		removable;
	byte		pwrctl;
	Device	*d;			/* attached device (if non-nil) */
	Hub		*hub;		/* non-nil if hub attached */
};

#pragma	varargck	type  "H"	Hub*

extern int verbose, debug;
